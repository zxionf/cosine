#pragma once

#include "font_atlas.hpp"

#include <string>
#include <vector>

namespace xel::graphics
{
struct FontCandidate
{
    std::string path;
    int face_index = 0;
};

FontCandidate find_cjk_font();

struct FontAtlasBuildOptions
{
    std::string font_path;
    int face_index = 0;
    int pixel_size = 16;
    int atlas_size = 2048;
    std::string coverage_file;
    std::vector<char32_t> extra_codepoints;
};

bool build_font_atlas(const FontAtlasBuildOptions& options, FontAtlas& out, std::string* error = nullptr);
} // namespace xel::graphics
