#include "font_atlas_builder.hpp"
#include "font_face.hpp"
#include "utf8.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_set>

namespace
{
std::vector<char32_t> common_punctuation()
{
    const std::string text = "，。！？：；“”‘’（）【】《》、…—·￥";
    std::vector<char32_t> out;
    std::size_t index = 0;
    while (index < text.size()) {
        out.push_back(xel::graphics::utf8_decode(text, index));
    }
    return out;
}

std::vector<char32_t> read_coverage_file(const std::string& path, std::vector<char32_t> existing)
{
    if (path.empty()) return existing;

    std::ifstream file(path, std::ios::binary);
    if (!file) return existing;

    const std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::size_t index = 0;
    while (index < content.size()) {
        const char32_t codepoint = xel::graphics::utf8_decode(content, index);
        if (codepoint >= 32) existing.push_back(codepoint);
    }
    return existing;
}

std::vector<char32_t> build_coverage(const xel::graphics::FontAtlasBuildOptions& options)
{
    std::vector<char32_t> codepoints;
    for (char32_t c = 32; c < 127; ++c) codepoints.push_back(c);

    const auto punctuation = common_punctuation();
    codepoints.insert(codepoints.end(), punctuation.begin(), punctuation.end());
    codepoints.insert(codepoints.end(), options.extra_codepoints.begin(), options.extra_codepoints.end());
    codepoints = read_coverage_file(options.coverage_file, std::move(codepoints));

    std::unordered_set<char32_t> seen;
    std::vector<char32_t> unique;
    unique.reserve(codepoints.size());
    for (char32_t codepoint : codepoints) {
        if (seen.insert(codepoint).second) unique.push_back(codepoint);
    }
    return unique;
}
} // namespace

namespace xel::graphics
{
FontCandidate find_cjk_font()
{
    if (const char* env = std::getenv("COSINE_CJK_FONT"); env && *env) {
        FontCandidate candidate{env, 0};
        if (const char* face = std::getenv("COSINE_CJK_FONT_FACE"); face && *face) {
            candidate.face_index = std::atoi(face);
        }
        std::error_code ec;
        if (std::filesystem::exists(candidate.path, ec)) return candidate;
    }

    const std::vector<FontCandidate> candidates = {
#ifdef _WIN32
        {"C:/Windows/Fonts/msyh.ttc", 0},
        {"C:/Windows/Fonts/msyhbd.ttc", 0},
        {"C:/Windows/Fonts/simhei.ttf", 0},
        {"C:/Windows/Fonts/simsun.ttc", 0},
#else
        {"/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc", 2},
        {"/usr/share/fonts/noto-cjk/NotoSansCJK-Medium.ttc", 2},
        {"/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Regular.otf", 0},
        {"/usr/share/fonts/wenquanyi/wqy-zenhei/wqy-zenhei.ttc", 0},
        {"/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc", 2},
#endif
    };

    std::error_code ec;
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate.path, ec)) return candidate;
    }
    return {};
}

bool build_font_atlas(const FontAtlasBuildOptions& options, FontAtlas& out, std::string* error)
{
    if (options.font_path.empty()) {
        if (error) *error = "font path is empty";
        return false;
    }
    if (options.atlas_size < 64 || options.pixel_size <= 0) {
        if (error) *error = "invalid atlas size or pixel size";
        return false;
    }

    FontFace face;
    if (!face.load(options.font_path, options.face_index, options.pixel_size)) {
        if (error) *error = "failed to open font: " + options.font_path;
        return false;
    }

    out.begin_generated(options.atlas_size, face.metrics(), true);

    const auto coverage = build_coverage(options);
    for (char32_t codepoint : coverage) {
        // 缺失字形（例如字体不包含某个标点）不是致命错误。
        (void)face.add_glyph(out, codepoint);
    }

    if (!out.valid()) {
        if (error) *error = "font atlas is empty";
        return false;
    }
    return true;
}
} // namespace xel::graphics
