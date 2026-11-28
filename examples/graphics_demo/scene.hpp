#pragma once

// ── 图形实验场景：只用 Canvas + FontAtlas 画内容，不碰 UI ─────────────
//
// 一个「页」= 一条注册表记录（id / 标题 / build），build 只依赖 Canvas 与参数；
// 新增实验内容 = 加一个 build_xxx 并在 table() 里加一行。

#include "canvas.hpp"

#include <string_view>
#include <vector>

namespace gdemo
{
// 场景内容的世界包围盒（以原点为中心，相机 fit 用它）。
inline constexpr glm::vec2 kContentSize{1680.0f, 1040.0f};

struct SceneOptions
{
    int page = 0;    // 当前页下标
    int density = 8; // batching 页的网格边长（4..24）
    bool grid = true;
    bool axes = true;
};

// HUD 上要显示的外部信息（都来自 renderer / 应用，不是场景自己算的）。
struct HudInfo
{
    float fps = 0.0f;
    float frame_ms = 0.0f;
    unsigned draw_calls = 0;
    unsigned vertices = 0;
    float ui_scale = 1.0f;
};

struct Page
{
    std::string_view id;
    std::string_view title;
    void (*build)(Canvas&, const SceneOptions&, float time) = nullptr;
};

const std::vector<Page>& pages();

// 按 id 查页下标；找不到返回 -1。
int page_index(std::string_view id);

// 画当前页：可选网格/坐标轴 + 该页内容。
void build_scene(Canvas& canvas, const SceneOptions& options, float time);

// 屏幕空间 HUD：标题、按键提示、缩放状态、帧统计。
void build_hud(Canvas& canvas, const SceneOptions& options, const HudInfo& info);
} // namespace gdemo
