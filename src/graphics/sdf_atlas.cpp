#include "sdf_atlas.hpp"

// stb_image 在 renderer.cpp 里已有一份实现；这里用 STB_IMAGE_STATIC 让函数
// 只在本编译单元可见，避免与后端重复定义符号。
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace xel::graphics
{
namespace
{
// 极简 JSON 读取：图集元数据是我们自己的生成器写的固定结构，
// 不值得为它引一个 JSON 库。只支持 "key": number / "key": "string"。
bool find_number(const std::string& json, const std::string& key, double& out)
{
    const std::string needle = "\"" + key + "\"";
    std::size_t pos = json.find(needle);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return false;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) ++pos;
    char* end = nullptr;
    const double value = std::strtod(json.c_str() + pos, &end);
    if (end == json.c_str() + pos) return false;
    out = value;
    return true;
}

std::string find_string(const std::string& json, const std::string& key)
{
    const std::string needle = "\"" + key + "\"";
    std::size_t pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos);
    if (pos == std::string::npos) return {};
    const std::size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return {};
    return json.substr(pos + 1, end - pos - 1);
}

// 解析 "icons": [ {"name": "...", "col": n, "row": n}, ... ]
bool parse_icons(const std::string& json, std::vector<SdfAtlas::Icon>& icons)
{
    const std::size_t list_start = json.find("\"icons\"");
    if (list_start == std::string::npos) return false;
    const std::size_t open = json.find('[', list_start);
    const std::size_t close = json.rfind(']');
    if (open == std::string::npos || close == std::string::npos || close < open) return false;

    const std::string body = json.substr(open, close - open);
    std::size_t pos = 0;
    while (true) {
        const std::size_t entry = body.find("\"name\"", pos);
        if (entry == std::string::npos) break;
        SdfAtlas::Icon icon;
        // name
        std::size_t quote = body.find('"', body.find(':', entry));
        const std::size_t name_end = body.find('"', quote + 1);
        if (quote == std::string::npos || name_end == std::string::npos) return false;
        icon.name = body.substr(quote + 1, name_end - quote - 1);
        // col / row（在同一个花括号内）
        const std::size_t brace_end = body.find('}', name_end);
        const std::string entry_body = body.substr(name_end, brace_end - name_end);
        double col = 0.0;
        double row = 0.0;
        if (!find_number(entry_body, "col", col) || !find_number(entry_body, "row", row)) return false;
        icon.col = static_cast<int>(col);
        icon.row = static_cast<int>(row);
        icons.push_back(std::move(icon));
        pos = brace_end == std::string::npos ? body.size() : brace_end;
    }
    return !icons.empty();
}
} // namespace

bool SdfAtlas::load(const std::string& png_path, const std::string& json_path, std::string* error)
{
    const auto fail = [error](std::string message) {
        if (error != nullptr) *error = std::move(message);
        return false;
    };

    std::ifstream meta_in(json_path);
    if (!meta_in) return fail("cannot open " + json_path);
    std::ostringstream meta;
    meta << meta_in.rdbuf();
    const std::string json = meta.str();

    double size = 0.0;
    double cell = 0.0;
    double px_range = 0.0;
    if (!find_number(json, "size", size) || !find_number(json, "cell", cell) ||
        !find_number(json, "px_range", px_range)) {
        return fail("bad metadata in " + json_path);
    }

    std::vector<Icon> icons;
    if (!parse_icons(json, icons)) return fail("no icons in " + json_path);

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* data = stbi_load(png_path.c_str(), &width, &height, &channels, 4);
    if (data == nullptr) return fail("cannot decode " + png_path);

    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
    std::copy(data, data + pixels.size(), pixels.begin());
    stbi_image_free(data);

    if (!adopt(width, height, static_cast<int>(cell), static_cast<float>(px_range), std::move(pixels),
               std::move(icons))) {
        return fail("metadata does not match " + png_path);
    }
    if (width < static_cast<int>(size)) {
        return fail("atlas smaller than declared size in " + json_path);
    }
    return true;
}

bool SdfAtlas::adopt(int width, int height, int cell, float px_range, std::vector<unsigned char> rgba,
                     std::vector<Icon> icons)
{
    if (width <= 0 || height <= 0 || cell <= 0) return false;
    if (rgba.size() != static_cast<std::size_t>(width) * height * 4) return false;
    for (const auto& icon : icons) {
        if (icon.col < 0 || icon.row < 0) return false;
        if ((icon.col + 1) * cell > width || (icon.row + 1) * cell > height) return false;
    }
    width_ = width;
    height_ = height;
    cell_ = cell;
    px_range_ = px_range;
    pixels_ = std::move(rgba);
    icons_ = std::move(icons);
    ++revision_;
    return true;
}

int SdfAtlas::icon_index(std::string_view name) const
{
    for (std::size_t i = 0; i < icons_.size(); ++i) {
        if (icons_[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

bool SdfAtlas::icon_uv(int index, glm::vec4& uv) const
{
    if (index < 0 || index >= icon_count() || !valid()) return false;
    const Icon& icon = icons_[static_cast<std::size_t>(index)];
    const float inv_w = 1.0f / static_cast<float>(width_);
    const float inv_h = 1.0f / static_cast<float>(height_);
    const float x0 = static_cast<float>(icon.col * cell_);
    const float y0 = static_cast<float>(icon.row * cell_);
    uv = {x0 * inv_w, y0 * inv_h, (x0 + static_cast<float>(cell_)) * inv_w,
          (y0 + static_cast<float>(cell_)) * inv_h};
    return true;
}
} // namespace xel::graphics
