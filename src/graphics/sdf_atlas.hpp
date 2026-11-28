#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace xel::graphics
{
// ── MTSDF 图标图集 ────────────────────────────────────────────────
//
// 由 tools/mtsdf_gen 烘焙（PNG + JSON），运行时只负责加载与查询：
//   * 像素格式固定 RGBA8 **UNORM**（距离是数据，不能走 sRGB）；
//   * RGB = MSDF 三通道，A = 真 SDF（描边/发光用）；
//   * 每个图标占一个 cell，UV 由 JSON 里的行列算出。
//
// 与 FontAtlas 一样是「一次性加载 + revision」，变化时由后端重新上传纹理。
class SdfAtlas
{
public:
    struct Icon
    {
        std::string name;
        int col = 0;
        int row = 0;
    };

    // 失败时返回 false 并写入 error；成功后 pixels() 是 width*height*4 的 RGBA8。
    bool load(const std::string& png_path, const std::string& json_path, std::string* error = nullptr);
    // 测试用：直接给像素与元数据（跳过文件 IO）。
    bool adopt(int width, int height, int cell, int margin, float px_range,
               std::vector<unsigned char> rgba, std::vector<Icon> icons);

    bool valid() const { return width_ > 0 && height_ > 0 && cell_ > 0; }
    int width() const { return width_; }
    int height() const { return height_; }
    int cell() const { return cell_; }
    // 图集烘焙时的距离范围（像素）；描边/发光需要它，AA 本身不需要。
    float px_range() const { return px_range_; }
    // cell 边缘到内容的空环宽度（像素，= padding + px_range）。
    // icon_uv() 已把它排除，所以按目标尺寸画出的图标不会被空环压小。
    int margin() const { return margin_; }
    uint64_t revision() const { return revision_; }
    const std::vector<unsigned char>& pixels() const { return pixels_; }
    const std::vector<Icon>& icons() const { return icons_; }

    int icon_count() const { return static_cast<int>(icons_.size()); }
    // 按名字查图标下标；找不到返回 -1。
    int icon_index(std::string_view name) const;

    // 图标在 atlas 里的 uv 包围盒 (u0, v0, u1, v1)；越界返回 false。
    bool icon_uv(int index, glm::vec4& uv) const;

private:
    int width_ = 0;
    int height_ = 0;
    int cell_ = 0;
    int margin_ = 0;
    float px_range_ = 0.0f;
    std::vector<unsigned char> pixels_;
    std::vector<Icon> icons_;
    uint64_t revision_ = 0;
};
} // namespace xel::graphics
