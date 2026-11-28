// mtsdf_gen：把内置几何烘焙成 MTSDF 图标图集。
//
// 为什么要内置几何而不是 SVG：
//   第一版只需验证「图集 → shader median 重建 → 1px AA」这条链路，
//   几何用代码写比引 SVG 解析器（nanosvg/msdfgen-ext）少一个依赖。
//   要换成 SVG 时，只改 make_shape 这一段即可。
//
// 输出：
//   <out>.png   图集（RGBA8，UNORM：RGB = MSDF 三通道，A = 真 SDF）
//   <out>.json  元数据（尺寸 / cell / padding / px_range / 每个图标的行列）
//
// 距离场由 msdfgen（MIT）生成：generateMTSDF 会把距离除以 px_range，输出到 [-1, 1]。

#include <msdfgen.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

namespace
{
constexpr int kDefaultCell = 96;
constexpr int kDefaultPadding = 8;
constexpr double kDefaultRange = 6.0;
constexpr int kColumns = 5;
// 折线逼近圆弧的段数：96px cell 下远超所需（弦高 << 0.1px）。
constexpr int kArcSegments = 48;

using Points = std::vector<msdfgen::Point2>;

// ── 内置几何（单位空间，y 向上；msdfgen 会自动 normalize）──────────
inline void add_edges(msdfgen::Contour& contour, const Points& points)
{
    for (std::size_t i = 0; i < points.size(); ++i) {
        contour.addEdge(msdfgen::EdgeHolder(points[i], points[(i + 1) % points.size()]));
    }
}
Points rounded_rect_points(double half_x, double half_y, double radius, int segments)
{
    Points points;
    const double hx = half_x - radius;
    const double hy = half_y - radius;
    constexpr double kPi = 3.14159265358979323846;
    // y 向上坐标系里的四个圆角（逆时针）：
    //   右下 270°→360°，右上 0°→90°，左上 90°→180°，左下 180°→270°
    // 相邻弧之间自然留下四条直边。
    const double centers[4][2] = {
        {hx, -hy},
        {hx, hy},
        {-hx, hy},
        {-hx, -hy},
    };
    const double starts[4] = {1.5 * kPi, 0.0, 0.5 * kPi, kPi};
    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i < segments; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(segments);
            const double angle = starts[corner] + t * kPi * 0.5;
            points.push_back({centers[corner][0] + radius * std::cos(angle),
                              centers[corner][1] + radius * std::sin(angle)});
        }
    }
    return points;
}

void build_rounded_rect(msdfgen::Shape& shape)
{
    add_edges(shape.addContour(), rounded_rect_points(0.9, 0.9, 0.34, kArcSegments));
}

void build_circle(msdfgen::Shape& shape)
{
    Points points;
    constexpr int kSegments = 128;
    constexpr double kPi = 3.14159265358979323846;
    for (int i = 0; i < kSegments; ++i) {
        const double angle = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(kSegments);
        points.push_back({0.9 * std::cos(angle), 0.9 * std::sin(angle)});
    }
    add_edges(shape.addContour(), points);
}

void build_play(msdfgen::Shape& shape)
{
    // 朝右的播放三角形（有明显尖角：MSDF 的意义就在这里）。
    add_edges(shape.addContour(), Points{
        {-0.55, -0.85},
        {0.85, 0.0},
        {-0.55, 0.85},
    });
}

// 朝右的尖括号粗线带（chevron）：外轮廓 + 内轮廓共 6 点，简单多边形、不自交。
// 左端面在 x = -0.85 上，从上到下：P5(0.45) → P0(0.85) 与 P3(-0.45) → P2(-0.85)。
void build_chevron(msdfgen::Shape& shape)
{
    add_edges(shape.addContour(), Points{
        {-0.85,  0.85}, // 左上（外）
        { 0.85,  0.00}, // 外尖
        {-0.85, -0.85}, // 左下（外）
        {-0.85, -0.45}, // 左下（内）
        { 0.25,  0.00}, // 内尖
        {-0.85,  0.45}, // 左上（内）
    });
}

void build_oilk(msdfgen::Shape& shape)
{
    // 朝右的 V 形箭头：两条粗线段的并集，靠折线描述外轮廓。
    const double t = 0.22; // 线宽
    add_edges(shape.addContour(), Points{
        {-0.7, 0.8}, {-0.2, 0.8}, {0.35, 0.2}, {0.1, 0.2},
        {0.35, -0.2}, {-0.2, -0.8}, {-0.7, -0.8}, {0.0, 0.0},
    });
}

void build_plus(msdfgen::Shape& shape)
{
    // 加号：12 点凹多边形（中心在内、四角在外），用来验证凹角与 MSDF 保尖角。
    add_edges(shape.addContour(), Points{
        { 0.30,  0.90}, { 0.30,  0.30}, { 0.90,  0.30}, { 0.90, -0.30},
        { 0.30, -0.30}, { 0.30, -0.90}, {-0.30, -0.90}, {-0.30, -0.30},
        {-0.90, -0.30}, {-0.90,  0.30}, {-0.30,  0.30}, {-0.30,  0.90},
    });
}

void build_arrow(msdfgen::Shape& shape)
{
    // 右箭头：杆 + 头（尖角 + 内凹角）。
    add_edges(shape.addContour(), Points{
        {-0.9, 0.22}, {0.1, 0.22}, {0.1, 0.62}, {0.9, 0.0},
        {0.1, -0.62}, {0.1, -0.22}, {-0.9, -0.22},
    });
}

void build_gear(msdfgen::Shape& shape)
{
    // 8 齿的简化齿轮：外轮廓由 8 段齿顶 + 8 段齿根构成。
    Points points;
    constexpr int kTeeth = 8;
    constexpr double kPi = 3.14159265358979323846;
    const double outer = 0.92;
    const double inner = 0.62;
    for (int i = 0; i < kTeeth; ++i) {
        const double base = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(kTeeth);
        const double step = 2.0 * kPi / static_cast<double>(kTeeth);
        const double half_tooth = step * 0.22;
        points.push_back({outer * std::cos(base - half_tooth), outer * std::sin(base - half_tooth)});
        points.push_back({outer * std::cos(base + half_tooth), outer * std::sin(base + half_tooth)});
        const double next = base + step;
        points.push_back({inner * std::cos(next - half_tooth * 1.6), inner * std::sin(next - half_tooth * 1.6)});
    }
    add_edges(shape.addContour(), points);
}

struct IconDef
{
    const char* name;
    void (*build)(msdfgen::Shape&);
};

const IconDef kIcons[] = {
    {"rounded_rect", &build_rounded_rect},
    {"circle", &build_circle},
    {"play", &build_play},
    {"chevron", &build_chevron},
    {"arrow", &build_arrow},
    {"plus", &build_plus},
    {"gear", &build_gear},
    {"oilk", &build_oilk},
};

// generateMTSDF 已经把距离经 DistanceMapping 归一化到 [0,1]（0.5 = 边界）。
unsigned char quantize(float value)
{
    return static_cast<unsigned char>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

int next_pow2(int value)
{
    int result = 64;
    while (result < value) result *= 2;
    return result;
}

struct Options
{
    std::string out = "assets/sdf/ui_icons";
    int cell = kDefaultCell;
    int padding = kDefaultPadding;
    double range = kDefaultRange;
};

bool parse_args(int argc, char** argv, Options& options)
{
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto value = [&](const char* label) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s needs a value\n", label);
                return {};
            }
            return argv[++i];
        };
        if (arg == "--out") {
            options.out = value("--out");
        } else if (arg == "--cell") {
            options.cell = std::stoi(value("--cell"));
        } else if (arg == "--padding") {
            options.padding = std::stoi(value("--padding"));
        } else if (arg == "--px-range") {
            options.range = std::stod(value("--px-range"));
        } else if (arg == "--help" || arg == "-h") {
            std::printf(
                "usage: mtsdf_gen [--out PATH] [--cell N] [--padding N] [--px-range F]\n"
                "  writes PATH.png and PATH.json\n");
            return false;
        } else {
            std::fprintf(stderr, "unknown argument: %s\n", arg.c_str());
            return false;
        }
        if (options.out.empty()) return false;
    }
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    Options options;
    if (!parse_args(argc, argv, options)) return argc > 1 ? 1 : 0;

    const int count = static_cast<int>(std::size(kIcons));
    const int columns = std::min(kColumns, count);
    const int rows = (count + columns - 1) / columns;
    const int atlas_size = next_pow2(std::max(columns, rows) * options.cell);

    if (options.padding * 2 >= options.cell) {
        std::fprintf(stderr, "padding too large for cell\n");
        return 1;
    }

    // 未使用的 cell 填成「外部」（RGB=1、A=1），这样误采样也不会画出黑色。
    std::vector<unsigned char> atlas(
        static_cast<std::size_t>(atlas_size) * atlas_size * 4, 255);
    msdfgen::Bitmap<float, 4> cell_bitmap(options.cell, options.cell);

    // 图形占据 cell 内的可用区域：留出 padding + px_range 的距离余量。
    const double margin = static_cast<double>(options.padding) + options.range;
    const double scale = static_cast<double>(options.cell) * 0.5 - margin;
    const double center = static_cast<double>(options.cell) * 0.5;

    std::vector<std::string> json_icons;
    for (int index = 0; index < count; ++index) {
        msdfgen::Shape shape;
        kIcons[index].build(shape);
        shape.normalize();
        // 方向交给下面的「中心像素自检 + reverse」处理，不调用 orientContours()。
        if (!shape.validate()) {
            std::fprintf(stderr, "%s: invalid shape\n", kIcons[index].name);
            return 1;
        }

        // y 向上 → 图集坐标：投影不翻转，写行时翻转（见下面的 row 计算）。
        // 注意：msdfgen 的 Projection::project(p) = scale * (p + translate)，
        // translate 在缩放**之前**生效（shape 坐标系），所以要传像素偏移 / scale。
        const msdfgen::Vector2 translation(center / scale, center / scale);
        const msdfgen::Projection projection(msdfgen::Vector2(scale, scale), translation);
        // 距离在 shape 空间计算，range 也要用 shape 单位（像素范围 / 缩放）。
        const double shape_range = options.range / scale;
        msdfgen::generateMTSDF(cell_bitmap, shape, projection, shape_range);

        // 轮廓方向约定在不同版本/输入下不一致，烘完再自检。
        // 方向自检：四角应在外部，且内部像素占比要合理（不空不满）。
        // 不能用「中心在内」：像 chevron 这种开口形状，cell 中心恰好落在
        // 开口的内侧（外部），会被误判为方向反了。
        // 四角 + 占比对凸/凹/开口形状都成立，对圆环也成立。
        const auto direction_ok = [&]() {
            const int e = options.cell - 1;
            const float corners[4] = {cell_bitmap(2, 2)[3], cell_bitmap(e - 2, 2)[3],
                                      cell_bitmap(2, e - 2)[3], cell_bitmap(e - 2, e - 2)[3]};
            const bool corners_outside = corners[0] > 0.5f && corners[1] > 0.5f &&
                                         corners[2] > 0.5f && corners[3] > 0.5f;

            int interior = 0;
            const int total = options.cell * options.cell;
            for (int y = 0; y < options.cell; ++y) {
                for (int x = 0; x < options.cell; ++x) {
                    if (cell_bitmap(x, y)[3] < 0.5f) ++interior;
                }
            }
            const double ratio = static_cast<double>(interior) / static_cast<double>(total);
            const bool fill_ok = ratio > 0.01 && ratio < 0.99;

            if (std::getenv("MTSDF_DEBUG")) {
                std::fprintf(stderr, "  diag corners=[%.2f %.2f %.2f %.2f] fill=%.1f%%\n",
                             corners[0], corners[1], corners[2], corners[3], ratio * 100.0);
            }
            return corners_outside && fill_ok;
        };
        if (!direction_ok()) {
            for (msdfgen::Contour& contour : shape.contours) contour.reverse();
            msdfgen::generateMTSDF(cell_bitmap, shape, projection, shape_range);
            if (!direction_ok()) {
                std::fprintf(stderr, "%s: contour direction is inconsistent\n",
                             kIcons[index].name);
                return 1;
            }
        }
        if (std::getenv("MTSDF_DEBUG")) {
            double bl = 0.0, bb = 0.0, br = 0.0, bt = 0.0;
            shape.bound(bl, bb, br, bt);
            const msdfgen::Point2 p0 = projection.project(msdfgen::Point2(bl, bb));
            const msdfgen::Point2 p1 = projection.project(msdfgen::Point2(br, bt));
            std::printf("  bounds [%.2f,%.2f]-[%.2f,%.2f] px [%.1f,%.1f]-[%.1f,%.1f] edges %d\n",
                        bl, bb, br, bt, p0.x, p0.y, p1.x, p1.y, shape.edgeCount());
            float lo = 1e9f;
            float hi = -1e9f;
            for (int yy = 0; yy < options.cell; ++yy) {
                for (int xx = 0; xx < options.cell; ++xx) {
                    const float value = cell_bitmap(xx, yy)[3];
                    lo = std::min(lo, value);
                    hi = std::max(hi, value);
                }
            }
            const float* c = cell_bitmap(options.cell / 2, options.cell / 2);
            const float* e = cell_bitmap(2, 2);
            std::printf("  a-range [%.3f, %.3f] center %.3f corner %.3f\n", lo, hi, c[3], e[3]);
        }

        const int col = index % columns;
        const int row = index / columns;
        const int origin_x = col * options.cell;
        const int origin_y = row * options.cell;
        for (int y = 0; y < options.cell; ++y) {
            const int dst_y = origin_y + (options.cell - 1 - y); // y 翻转
            for (int x = 0; x < options.cell; ++x) {
                const float* pixel = cell_bitmap(x, y);
                const std::size_t offset =
                    (static_cast<std::size_t>(dst_y) * atlas_size + origin_x + x) * 4;
                atlas[offset + 0] = quantize(pixel[0]);
                atlas[offset + 1] = quantize(pixel[1]);
                atlas[offset + 2] = quantize(pixel[2]);
                atlas[offset + 3] = quantize(pixel[3]);
            }
        }

        char entry[256];
        std::snprintf(entry, sizeof(entry),
                      "    {\"name\": \"%s\", \"col\": %d, \"row\": %d}", kIcons[index].name,
                      col, row);
        json_icons.emplace_back(entry);
        std::printf("baked %-12s cell (%d, %d)\n", kIcons[index].name, col, row);
    }

    const std::string png_path = options.out + ".png";
    const std::string json_path = options.out + ".json";

    if (!stbi_write_png(png_path.c_str(), atlas_size, atlas_size, 4, atlas.data(),
                        atlas_size * 4)) {
        std::fprintf(stderr, "cannot write %s\n", png_path.c_str());
        return 1;
    }

    std::ofstream json(json_path);
    if (!json) {
        std::fprintf(stderr, "cannot write %s\n", json_path.c_str());
        return 1;
    }
    json << "{\n"
         << "  \"atlas\": \"" << (png_path.substr(png_path.find_last_of('/') + 1)) << "\",\n"
         << "  \"size\": " << atlas_size << ",\n"
         << "  \"cell\": " << options.cell << ",\n"
         << "  \"padding\": " << options.padding << ",\n"
         // margin = padding + px_range：cell 边缘到内容的空环。渲染时 UV 必须
         // 跳过它（见 SdfAtlas::icon_uv），否则图标会比目标尺寸小一圈。
         << "  \"margin\": " << static_cast<int>(margin) << ",\n"
         << "  \"px_range\": " << options.range << ",\n"
         << "  \"icons\": [\n";
    for (std::size_t i = 0; i < json_icons.size(); ++i) {
        json << json_icons[i] << (i + 1 == json_icons.size() ? "\n" : ",\n");
    }
    json << "  ]\n}\n";

    std::printf("wrote %s (%dx%d) and %s\n", png_path.c_str(), atlas_size, atlas_size,
                json_path.c_str());
    return 0;
}
