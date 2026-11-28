#include "scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace gdemo
{
namespace
{
// ── 固定色板（不依赖主题：这一层只谈绘制）─────────────────────────
constexpr glm::vec4 kInk{0.88f, 0.90f, 0.94f, 1.0f};
constexpr glm::vec4 kMuted{0.58f, 0.62f, 0.70f, 1.0f};
constexpr glm::vec4 kPanel{0.14f, 0.15f, 0.19f, 1.0f};
constexpr glm::vec4 kBlue{0.36f, 0.62f, 1.00f, 1.00f};
constexpr glm::vec4 kGreen{0.36f, 0.84f, 0.58f, 1.00f};
constexpr glm::vec4 kOrange{1.00f, 0.62f, 0.30f, 1.00f};
constexpr glm::vec4 kPink{0.95f, 0.40f, 0.66f, 1.00f};
constexpr glm::vec4 kViolet{0.62f, 0.48f, 1.00f, 1.00f};
constexpr glm::vec4 kCyan{0.32f, 0.82f, 0.86f, 1.00f};
constexpr glm::vec4 kYellow{0.98f, 0.84f, 0.36f, 1.00f};
// 亮底上的文字（渐变卡片左上角）用深色，否则看不出。
constexpr glm::vec4 kDarkInk{0.05f, 0.06f, 0.08f, 0.92f};

const glm::vec4 kPalette[7] = {kBlue, kGreen, kOrange, kPink, kViolet, kCyan, kYellow};
const char* const kPaletteNames[7] = {"blue", "green", "orange", "pink", "violet", "cyan", "yellow"};

std::string label(const char* prefix, float value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%s %.1f", prefix, static_cast<double>(value));
    return buffer;
}

// 整数计数单独一个格式，避免 "cells 64.0"。
std::string count_label(const char* prefix, int value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%s %d", prefix, value);
    return buffer;
}

std::vector<glm::vec2> sample(int count, const std::function<glm::vec2(float)>& curve)
{
    std::vector<glm::vec2> points;
    points.reserve(static_cast<std::size_t>(count) + 1);
    for (int i = 0; i <= count; ++i) {
        points.push_back(curve(static_cast<float>(i) / static_cast<float>(count)));
    }
    return points;
}

glm::vec2 quadratic(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, float t)
{
    const float u = 1.0f - t;
    return u * u * p0 + 2.0f * u * t * p1 + t * t * p2;
}

glm::vec2 cubic(glm::vec2 p0, glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, float t)
{
    const float u = 1.0f - t;
    const float uu = u * u;
    const float tt = t * t;
    return uu * u * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + tt * t * p3;
}

// ── 页 1：基本图元 ────────────────────────────────────────────────
void build_shapes(Canvas& c, const SceneOptions&, float)
{
    c.text({-780.0f, -500.0f}, 22.0f, "矩形 / 圆 / 环 / 圆角 / 三角 / 描边 / 渐变", kInk);

    // 色板：铺满一行（世界宽 1524）。
    for (int i = 0; i < 7; ++i) {
        const float x = -780.0f + static_cast<float>(i) * 224.0f;
        c.rect({x, -450.0f}, {180.0f, 150.0f}, kPalette[i]);
        c.text({x, -288.0f}, 14.0f, kPaletteNames[i], kMuted);
    }

    // 半透明叠加：先铺不透明底板，否则网格会透上来干扰观感。
    c.text({-780.0f, -252.0f}, 14.0f, "alpha 0.35 × 4（叠加顺序 = 提交顺序）", kMuted);
    c.rect({-780.0f, -232.0f}, {1560.0f, 200.0f}, {0.10f, 0.11f, 0.14f, 1.0f});
    for (int i = 0; i < 4; ++i) {
        c.rect({-760.0f + static_cast<float>(i) * 366.0f, -222.0f}, {420.0f, 180.0f},
               {0.36f, 0.62f, 1.00f, 0.35f});
    }

    c.circle({-620.0f, 120.0f}, 96.0f, kBlue);
    c.ring({-380.0f, 120.0f}, 96.0f, 18.0f, kGreen);
    c.rounded_rect({-180.0f, 24.0f}, {192.0f, 192.0f}, 32.0f, kOrange);
    c.triangle({60.0f, 216.0f}, {180.0f, 24.0f}, {300.0f, 216.0f}, kPink);
    c.outline({360.0f, 24.0f}, {192.0f, 192.0f}, 4.0f, kInk);
    c.gradient_rect({600.0f, 24.0f}, {180.0f, 192.0f}, kViolet, kYellow);

    c.text({-620.0f, 250.0f}, 14.0f, "circle", kMuted);
    c.text({-380.0f, 250.0f}, 14.0f, "ring", kMuted);
    c.text({-180.0f, 250.0f}, 14.0f, "rounded", kMuted);
    c.text({60.0f, 250.0f}, 14.0f, "triangle", kMuted);
    c.text({360.0f, 250.0f}, 14.0f, "outline", kMuted);
    c.text({600.0f, 250.0f}, 14.0f, "gradient", kMuted);

    // 圆角半径 / 描边宽度阶梯：两排对照（几何细分随屏幕尺寸变化）。
    // 同样铺不透明底板，避免网格线穿过空心矩形造成误读。
    c.rect({-780.0f, 306.0f}, {1560.0f, 156.0f}, {0.10f, 0.11f, 0.14f, 1.0f});
    c.text({-780.0f, 320.0f}, 14.0f, "圆角半径 8 / 32 / 45（已到半边高）：角上扇形段数随屏幕半径增加", kMuted);
    const float radii[3] = {8.0f, 32.0f, 45.0f};
    for (int i = 0; i < 3; ++i) {
        c.rounded_rect({-780.0f + static_cast<float>(i) * 200.0f, 350.0f}, {170.0f, 90.0f},
                       radii[i], kViolet);
    }
    c.text({-180.0f, 320.0f}, 14.0f, "描边宽度 1 / 4 / 10（屏幕下限 0.75px）", kMuted);
    const float outlines[3] = {1.0f, 4.0f, 10.0f};
    for (int i = 0; i < 3; ++i) {
        c.outline({-180.0f + static_cast<float>(i) * 200.0f, 350.0f}, {170.0f, 90.0f},
                  outlines[i], kCyan);
    }
    c.text({-780.0f, 470.0f}, 14.0f,
           "放大/缩小看：线宽、圆角段数、圆的段数都按屏幕尺寸重算", kMuted);
}

// ── 页 2：线段与描边 ──────────────────────────────────────────────
void build_lines(Canvas& c, const SceneOptions&, float time)
{
    c.text({-780.0f, -500.0f}, 22.0f, "线宽 / 虚线 / 折线 / 采样曲线", kInk);

    // 线宽对比需要干净背景：铺一块不透明底板，避免网格线被误读成线段。
    c.rect({-790.0f, -445.0f}, {1580.0f, 330.0f}, {0.10f, 0.11f, 0.14f, 1.0f});

    for (int i = 0; i < 6; ++i) {
        const float width = std::pow(2.0f, static_cast<float>(i));
        const float y = -420.0f + static_cast<float>(i) * 46.0f;
        c.line({-700.0f, y}, {-320.0f, y}, width, kInk);
        c.text({-780.0f, y - 11.0f}, 14.0f, label("width", width), kMuted);
    }

    c.dashed_line({-780.0f, -100.0f}, {-320.0f, -100.0f}, 3.0f, 16.0f, 10.0f, kBlue);
    c.dashed_line({-780.0f, -70.0f}, {-320.0f, -70.0f}, 2.0f, 4.0f, 4.0f, kBlue);
    c.text({-300.0f, -112.0f}, 14.0f, "dash 16/10", kMuted);
    c.text({-300.0f, -82.0f}, 14.0f, "dash 4/4", kMuted);

    // 折线：锯齿（观察尖角处的接缝与线宽）。
    std::vector<glm::vec2> zigzag;
    for (int i = 0; i < 24; ++i) {
        zigzag.push_back({-780.0f + static_cast<float>(i) * 20.0f,
                          -20.0f + ((i % 2 == 0) ? -40.0f : 40.0f)});
    }
    c.polyline(zigzag, 3.0f, kGreen);
    c.text({-300.0f, -30.0f}, 14.0f, "polyline 24 pts", kMuted);

    // 正弦：采样越密越平滑，线宽 3。
    const auto sine = sample(400, [time](float t) {
        const float x = -780.0f + t * 1560.0f;
        return glm::vec2{x, 140.0f + 70.0f * std::sin(t * 12.0f + time * 0.8f)};
    });
    c.polyline(sine, 3.0f, kOrange);
    c.text({-780.0f, 40.0f}, 14.0f, "sine · 400 samples（随时间变化）", kMuted);

    c.rect({-790.0f, 285.0f}, {1580.0f, 190.0f}, {0.10f, 0.11f, 0.14f, 1.0f});
    // 细线：从粗到细排列，缩放实验里能看清「细到一定程度就退到 0.75px 下限」。
    const float widths[5] = {3.0f, 2.0f, 1.5f, 1.0f, 0.5f};
    for (int i = 0; i < 5; ++i) {
        const float y = 300.0f + static_cast<float>(i) * 26.0f;
        c.line({-700.0f, y}, {780.0f, y}, widths[i], kInk);
        c.text({-780.0f, y - 10.0f}, 12.0f, label("width", widths[i]), kMuted);
    }
    c.text({-780.0f, 450.0f}, 14.0f,
           "同一条线在 0.5 / 1 / 1.5 / 2 / 3 世界宽度下：屏幕宽度下限 0.75px", kMuted);
}

// ── 页 3：曲线与细分 ──────────────────────────────────────────────
void build_curves(Canvas& c, const SceneOptions&, float time)
{
    c.text({-780.0f, -500.0f}, 22.0f, "贝塞尔 / 极坐标 / 细分段数", kInk);

    const glm::vec2 p0{-780.0f, -360.0f};
    const glm::vec2 p1{-620.0f, 220.0f};
    const glm::vec2 p2{-460.0f, -360.0f};
    const int steps[3] = {8, 24, 96};
    const glm::vec4 colors[3] = {kBlue, kGreen, kPink};
    // 从细到粗、从密到疏地叠：8 段（粗、粉）最后画，折角才看得见。
    for (int i = 2; i >= 0; --i) {
        const auto curve = sample(steps[i], [&](float t) { return quadratic(p0, p1, p2, t); });
        c.polyline(curve, i == 0 ? 3.0f : 1.5f, colors[i]);
    }
    c.text({-780.0f, -390.0f}, 14.0f, "quadratic · 96 / 24 / 8 段叠加（8 段最粗）", kMuted);

    const glm::vec2 q0{-380.0f, -360.0f};
    const glm::vec2 q1{-300.0f, 260.0f};
    const glm::vec2 q2{-140.0f, -400.0f};
    const glm::vec2 q3{-40.0f, -360.0f};
    for (int i = 2; i >= 0; --i) {
        const auto curve = sample(steps[i], [&](float t) { return cubic(q0, q1, q2, q3, t); });
        c.polyline(curve, i == 0 ? 3.0f : 1.5f, colors[i]);
    }
    c.text({-380.0f, -390.0f}, 14.0f, "cubic · 96 / 24 / 8 段叠加", kMuted);

    // 玫瑰线 r = R·cos(3θ)，720 段。
    const glm::vec2 rose_center{220.0f, -140.0f};
    const auto rose = sample(720, [&](float t) {
        const float theta = t * 6.2831853f;
        const float r = 170.0f * std::cos(3.0f * theta);
        return rose_center + glm::vec2{r * std::cos(theta), r * std::sin(theta)};
    });
    c.polyline(rose, 2.0f, kViolet);
    c.text({100.0f, -390.0f}, 14.0f, "rose r = 170·cos 3θ", kMuted);

    // 阿基米德螺旋：800 段，随 time 缓慢旋转。
    const glm::vec2 spiral_center{600.0f, -140.0f};
    const auto spiral = sample(800, [&](float t) {
        const float theta = t * 18.8495559f + time * 0.6f;
        const float r = 12.0f + 9.0f * theta;
        return spiral_center + glm::vec2{r * std::cos(theta), r * std::sin(theta)};
    });
    c.polyline(spiral, 2.0f, kCyan);
    c.text({520.0f, -390.0f}, 14.0f, "spiral · 800 段", kMuted);

    // 多边形近似圆：段数决定「圆不圆」，与 Canvas::circle 的自适应段数对照。
    const int segment_counts[3] = {12, 32, 96};
    for (int i = 0; i < 3; ++i) {
        const glm::vec2 center{-620.0f + static_cast<float>(i) * 260.0f, 240.0f};
        std::vector<glm::vec2> ring;
        const int count = segment_counts[i];
        for (int k = 0; k <= count; ++k) {
            const float theta = 6.2831853f * static_cast<float>(k) / static_cast<float>(count);
            ring.push_back(center + glm::vec2{90.0f * std::cos(theta), 90.0f * std::sin(theta)});
        }
        c.polyline(ring, 2.0f, kYellow);
        c.text({center.x - 60.0f, center.y + 110.0f}, 14.0f,
               label("segments", static_cast<float>(count)), kMuted);
    }
    c.text({-620.0f, 380.0f}, 14.0f,
           "Canvas::circle 的段数按屏幕半径自适应（放大后更圆）", kMuted);
}

// ── 页 4：文字 ────────────────────────────────────────────────────
void build_text(Canvas& c, const SceneOptions&, float)
{
    // 文字页铺半透明底板：网格淡出，字仍清晰（也顺便验证 alpha 叠在网格上）。
    // 底板必须先于标题，否则会把标题盖暗。
    c.rect({-840.0f, -520.0f}, {1680.0f, 1040.0f}, {0.07f, 0.08f, 0.10f, 0.86f});
    c.text({-780.0f, -500.0f}, 22.0f, "字号阶梯 / 对齐 / 多行 / 缩放栅格化", kInk);

    const float sizes[6] = {12.0f, 16.0f, 22.0f, 30.0f, 42.0f, 58.0f};
    float y = -430.0f;
    for (const float size : sizes) {
        c.text({-780.0f, y}, size, "XEL graphics · 0123456789", kInk);
        c.text({-100.0f, y}, 12.0f, label("size", size), kMuted);
        y += size + 14.0f;
    }

    // 对齐：同一右边界，用 text_width 反推起点。
    const float right = 780.0f;
    const char* const lines[3] = {"right aligned", "右对齐文本", "text_width() 反推起点"};
    y = -430.0f;
    for (const char* line : lines) {
        const float width = c.text_width(line, 18.0f);
        c.text({right - width, y}, 18.0f, line, kBlue);
        y += 30.0f;
    }

    c.text({-780.0f, 120.0f}, 18.0f, "多行（\\n 换行）：", kMuted);
    c.text({-780.0f, 150.0f}, 18.0f,
           "第一行：字号与行高成比例\n第二行：缩放时整体一起放大\n第三行：字形按整数像素尺寸重新栅格化",
           kInk);

    c.text({300.0f, 120.0f}, 96.0f, "Aa", kYellow);
    c.text({300.0f, 250.0f}, 14.0f, "96px 特写：放大后仍按实际字号栅格", kMuted);

    // 半透明文字叠在色块上：与 Rectangle / Sprite 共用同一套预乘 alpha。
    c.text({-780.0f, 320.0f}, 14.0f, "半透明文字叠在色块上（预乘 alpha，不是单纯降不透明度）", kMuted);
    c.rect({-780.0f, 350.0f}, {740.0f, 110.0f}, kViolet);
    c.text({-764.0f, 366.0f}, 46.0f, "alpha 0.5", {0.0f, 0.0f, 0.0f, 0.5f});
    c.rect({20.0f, 350.0f}, {740.0f, 110.0f}, kYellow);
    c.text({36.0f, 366.0f}, 46.0f, "alpha 0.5", {0.0f, 0.0f, 0.0f, 0.5f});

    c.text({-780.0f, 485.0f}, 14.0f,
           "文字与图元共用同一套预乘 alpha，颜色一致", kMuted);
}

// ── 页 5：批处理与裁剪 ────────────────────────────────────────────
void build_batching(Canvas& c, const SceneOptions& options, float)
{
    const int n = std::clamp(options.density, 4, 24);
    c.text({-780.0f, -500.0f}, 22.0f, "批处理 / 裁剪：格子越多，draw call 不变", kInk);

    // 同槽、同 clip 的大量小方块 → 合并进一个 batch。
    const float cell = 660.0f / static_cast<float>(n);
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            const glm::vec4 base = ((row + col) % 2 == 0) ? kBlue : kPanel;
            const float shade = 0.55f + 0.45f * static_cast<float>(col + row) / (2.0f * n);
            const glm::vec4 color{base.r * shade, base.g * shade, base.b * shade, 1.0f};
            c.rect({-780.0f + static_cast<float>(col) * cell, -440.0f + static_cast<float>(row) * cell},
                   {cell - 2.0f, cell - 2.0f}, color);
        }
    }
    c.text({-780.0f, -440.0f + static_cast<float>(n) * cell + 10.0f}, 14.0f,
           count_label("cells", n * n), kMuted);

    // 三个带裁剪的卡片：每个 clip 会切出一个新 batch（右侧渐变条溢出被裁掉）。
    const glm::vec4 colors[3] = {kGreen, kOrange, kPink};
    for (int i = 0; i < 3; ++i) {
        const float top = -440.0f + static_cast<float>(i) * 220.0f;
        const glm::vec2 origin{60.0f, top};
        c.push_clip(origin, {700.0f, 190.0f});
        c.gradient_rect(origin, {900.0f, 190.0f}, colors[i], kPanel);
        c.text({origin.x + 16.0f, origin.y + 16.0f}, 18.0f,
               label("clip card", static_cast<float>(i + 1)), kDarkInk);
        c.pop_clip();
        c.outline(origin, {700.0f, 190.0f}, 1.0f, kMuted);
    }
    c.text({60.0f, 236.0f}, 14.0f, "渐变按 900 宽画，超出 700 的部分被 scissor 裁掉",
           kMuted);

    // 对照组：同一渐变不裁剪 —— 溢出卡片边框的部分照样画出来。
    const glm::vec2 raw{60.0f, 320.0f};
    c.gradient_rect(raw, {800.0f, 150.0f}, kBlue, kPanel);
    c.outline(raw, {700.0f, 150.0f}, 1.0f, kMuted);
    c.text({raw.x + 16.0f, raw.y + 16.0f}, 18.0f, "no clip · 渐变 800 / 框 700", kDarkInk);
    c.text({raw.x, raw.y + 164.0f}, 14.0f,
           "同一渐变不裁剪：右侧溢出 100 照样画出 —— 裁剪切的是 batch，不是几何", kMuted);
}

// ── 注册表 ────────────────────────────────────────────────────────
const std::vector<Page>& table()
{
    static const std::vector<Page> kPages = {
        {"shapes", "基本图元", &build_shapes},
        {"lines", "线段与描边", &build_lines},
        {"curves", "曲线与细分", &build_curves},
        {"text", "文字", &build_text},
        {"batching", "批处理与裁剪", &build_batching},
    };
    return kPages;
}

void build_overlay_grid(Canvas& c, const SceneOptions& options)
{
    const glm::vec2 half = kContentSize * 0.5f;
    if (options.grid) {
        c.grid({-half.x, -half.y}, half, 100.0f, {0.32f, 0.36f, 0.44f, 0.22f});
        c.grid({-half.x, -half.y}, half, 500.0f, {0.44f, 0.50f, 0.60f, 0.34f});
    }
    if (options.axes) {
        c.line({-half.x, 0.0f}, {half.x, 0.0f}, 1.0f, {0.55f, 0.60f, 0.70f, 0.75f});
        c.line({0.0f, -half.y}, {0.0f, half.y}, 1.0f, {0.55f, 0.60f, 0.70f, 0.75f});
    }
}
} // namespace

const std::vector<Page>& pages()
{
    return table();
}

int page_index(std::string_view id)
{
    const auto& list = table();
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

void build_scene(Canvas& canvas, const SceneOptions& options, float time)
{
    // 自己的背景（屏幕空间）：不借 UI 主题的 clear color。
    canvas.screen_rect({0.0f, 0.0f}, canvas.viewport(), {0.07f, 0.08f, 0.10f, 1.0f});
    build_overlay_grid(canvas, options);

    const auto& list = table();
    const int index = std::clamp(options.page, 0, static_cast<int>(list.size()) - 1);
    const Page& page = list[static_cast<std::size_t>(index)];
    if (page.build != nullptr) page.build(canvas, options, time);
}

void build_hud(Canvas& canvas, const SceneOptions& options, const HudInfo& info)
{
    const glm::vec2 vp = canvas.viewport();
    if (vp.x < 200.0f || vp.y < 120.0f) return;

    const auto& list = table();
    const int index = std::clamp(options.page, 0, static_cast<int>(list.size()) - 1);
    const Page& page = list[static_cast<std::size_t>(index)];

    // 顶部条：标题 + 操作提示 + 右侧缩放读数。
    canvas.screen_rect({0.0f, 0.0f}, {vp.x, 58.0f}, {0.05f, 0.06f, 0.08f, 0.86f});
    canvas.screen_text({16.0f, 9.0f}, 18.0f,
                       "graphics demo · " + std::string{page.title}, kInk);
    canvas.screen_text({16.0f, 33.0f}, 12.0f,
                       "1-5 切页 · 滚轮缩放 · 左键拖动平移 · 0 适配 · +/- 缩放 · "
                       "G 网格 · H HUD · [ ] 密度 · Esc 退出",
                       kMuted);

    char zoom_text[96];
    std::snprintf(zoom_text, sizeof(zoom_text), "zoom %.3fx", static_cast<double>(canvas.camera().zoom));
    const float zoom_x = vp.x - 16.0f - canvas.text_width(zoom_text, 13.0f) - 0.0f;
    canvas.screen_text({std::max(16.0f, zoom_x), 12.0f}, 13.0f, zoom_text, kYellow);
    char pan_text[96];
    std::snprintf(pan_text, sizeof(pan_text), "pan (%.0f, %.0f)",
                  static_cast<double>(canvas.camera().pan.x),
                  static_cast<double>(canvas.camera().pan.y));
    canvas.screen_text({std::max(16.0f, zoom_x), 32.0f}, 12.0f, pan_text, kMuted);

    // 底部状态条：帧统计（来自 renderer）+ 画布自己的顶点计数。
    canvas.screen_rect({0.0f, vp.y - 26.0f}, {vp.x, 26.0f}, {0.05f, 0.06f, 0.08f, 0.86f});
    char status[512];
    std::snprintf(status, sizeof(status),
                  "fps %.0f   frame %.2f ms   draw %u   verts %u   batches %u   ui_scale %.2f   "
                  "cells %d   %s",
                  static_cast<double>(info.fps), static_cast<double>(info.frame_ms),
                  info.draw_calls, info.vertices,
                  static_cast<unsigned>(canvas.batch_count()),
                  static_cast<double>(info.ui_scale), options.density * options.density,
                  options.grid ? "grid on" : "grid off");
    canvas.screen_text({16.0f, vp.y - 21.0f}, 12.0f, status, kMuted);
}
} // namespace gdemo
