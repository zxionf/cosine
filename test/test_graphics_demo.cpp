// graphics_demo 的纯逻辑测试：相机换算 + 画布顶点/批次 + 场景注册表。
// 不需要窗口、GPU 或字体图集（atlas 为 nullptr 时文字入口直接跳过）。

#include "camera.hpp"
#include "canvas.hpp"
#include "scene.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
int failures = 0;

void check(bool condition, const std::string& name, const std::string& detail = {})
{
    if (condition) {
        std::cout << "ok      " << name << '\n';
        return;
    }
    ++failures;
    std::cout << "FAIL    " << name;
    if (!detail.empty()) std::cout << "  [" << detail << ']';
    std::cout << '\n';
}

bool near(float a, float b, float epsilon = 1e-4f)
{
    return std::fabs(a - b) <= epsilon;
}

bool near(glm::vec2 a, glm::vec2 b, float epsilon = 1e-4f)
{
    return near(a.x, b.x, epsilon) && near(a.y, b.y, epsilon);
}

std::string read_file(const fs::path& path)
{
    std::ifstream in(path);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

// 一个默认相机：世界原点在视口中心，缩放 1。
gdemo::Camera centered(glm::vec2 viewport = {800.0f, 600.0f})
{
    gdemo::Camera camera;
    camera.pan = viewport * 0.5f;
    return camera;
}

// 把画布顶点展平，供缩放/坐标比较（批次顺序 = 提交顺序）。
std::vector<xel::graphics::Vertex> flatten(const gdemo::Canvas& canvas)
{
    std::vector<xel::graphics::Vertex> vertices;
    for (const auto& batch : canvas.batches()) {
        vertices.insert(vertices.end(), batch.vertices.begin(), batch.vertices.end());
    }
    return vertices;
}

// ── 相机 ──────────────────────────────────────────────────────────
void test_camera_roundtrip()
{
    gdemo::Camera camera = centered();
    camera.zoom = 2.5f;
    const glm::vec2 world{37.0f, -12.0f};
    const glm::vec2 screen = camera.to_screen(world);
    check(near(camera.to_world(screen), world), "camera: to_world(to_screen(p)) == p");
    check(near(camera.length_to_screen(10.0f), 25.0f), "camera: length scales with zoom");
    check(near(camera.length_to_world(25.0f), 10.0f), "camera: length_to_world inverts");
}

void test_camera_zoom_anchor()
{
    gdemo::Camera camera = centered();
    const glm::vec2 anchor{123.0f, 456.0f};
    const glm::vec2 world_under_anchor = camera.to_world(anchor);

    for (const float factor : {1.25f, 0.5f, 3.0f, 0.8f}) {
        camera.zoom_at(anchor, factor);
        const glm::vec2 projected = camera.to_screen(world_under_anchor);
        check(near(projected, anchor, 1e-3f), "camera: zoom_at keeps the anchored world point fixed",
              "factor=" + std::to_string(factor));
    }
}

void test_camera_zoom_clamp()
{
    gdemo::Camera camera = centered();
    camera.zoom_at({400.0f, 300.0f}, 1e6f);
    check(near(camera.zoom, gdemo::Camera::kMaxZoom), "camera: zoom clamped to max");
    camera.zoom_at({400.0f, 300.0f}, 1e-9f);
    check(near(camera.zoom, gdemo::Camera::kMinZoom), "camera: zoom clamped to min");
    const float before = camera.zoom;
    camera.zoom_at({400.0f, 300.0f}, 0.0f);
    check(near(camera.zoom, before), "camera: non-positive factor is ignored");
}

void test_camera_fit()
{
    gdemo::Camera camera;
    const glm::vec2 viewport{1280.0f, 840.0f};
    camera.fit(viewport, gdemo::kContentSize);
    check(camera.zoom > 0.0f, "camera: fit produces a positive zoom");

    // 内容四角都在视口内（含边距容差）。
    const glm::vec2 half = gdemo::kContentSize * 0.5f;
    const glm::vec4 bounds = camera.screen_bounds(-half, gdemo::kContentSize);
    check(bounds.x >= -0.5f && bounds.y >= -0.5f, "camera: fitted content starts inside the viewport");
    check(bounds.x + bounds.z <= viewport.x + 0.5f && bounds.y + bounds.w <= viewport.y + 0.5f,
          "camera: fitted content ends inside the viewport");
    check(near(camera.to_world(viewport * 0.5f), glm::vec2{0.0f, 0.0f}),
          "camera: fit centers the world origin");
}

void test_camera_pan()
{
    gdemo::Camera camera = centered();
    const glm::vec2 world{50.0f, 50.0f};
    const glm::vec2 before = camera.to_screen(world);
    camera.pan_by({30.0f, -20.0f});
    check(near(camera.to_screen(world), before + glm::vec2{30.0f, -20.0f}),
          "camera: pan shifts screen positions by the same delta");
    check(near(camera.to_world(before + glm::vec2{30.0f, -20.0f}), world),
          "camera: pan does not change world coordinates");
}

// ── 画布 ──────────────────────────────────────────────────────────
void test_canvas_rect()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered();
    canvas.begin(nullptr, camera, {800.0f, 600.0f});
    canvas.rect({0.0f, 0.0f}, {100.0f, 50.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
    check(canvas.vertex_count() == 4, "canvas: a rect emits 4 vertices");
    check(canvas.index_count() == 6, "canvas: a rect emits 6 indices");
    check(canvas.batch_count() == 1, "canvas: a rect is one batch");
}

void test_canvas_batches_merge()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered();
    canvas.begin(nullptr, camera, {800.0f, 600.0f});
    for (int i = 0; i < 64; ++i) {
        canvas.rect({static_cast<float>(i) * 4.0f, 0.0f}, {3.0f, 3.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
    }
    check(canvas.batch_count() == 1, "canvas: same clip + slot merges into one batch",
          "batches=" + std::to_string(canvas.batch_count()));
    check(canvas.vertex_count() == 64 * 4, "canvas: merged batch keeps all vertices");

    canvas.push_clip({0.0f, 0.0f}, {50.0f, 50.0f});
    canvas.rect({0.0f, 0.0f}, {10.0f, 10.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
    canvas.pop_clip();
    check(canvas.batch_count() == 2, "canvas: a clip splits the draw into a new batch",
          "batches=" + std::to_string(canvas.batch_count()));

    const auto& batches = canvas.batches();
    const glm::vec4 scissor = batches.size() > 1 ? batches[1].scissor : glm::vec4{0.0f};
    check(near(scissor, glm::vec4{400.0f, 300.0f, 50.0f, 50.0f}),
          "canvas: clip becomes the batch scissor",
          "scissor=" + std::to_string(scissor.x) + "," + std::to_string(scissor.y) + "," +
              std::to_string(scissor.z) + "," + std::to_string(scissor.w));
}

void test_canvas_screen_space_is_not_zoomed()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered();
    camera.zoom = 4.0f;
    canvas.begin(nullptr, camera, {800.0f, 600.0f});
    canvas.rect({0.0f, 0.0f}, {10.0f, 10.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
    const auto zoomed = flatten(canvas);

    gdemo::Canvas hud;
    hud.begin(nullptr, camera, {800.0f, 600.0f});
    hud.screen_rect({0.0f, 0.0f}, {10.0f, 10.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
    const auto fixed = flatten(hud);

    if (zoomed.size() < 4 || fixed.size() < 4) {
        check(false, "canvas: screen space snapshot available");
        return;
    }
    const float zoomed_width = std::fabs(zoomed[1].pos.x - zoomed[0].pos.x);
    const float fixed_width = std::fabs(fixed[1].pos.x - fixed[0].pos.x);
    check(near(zoomed_width, 40.0f, 0.01f), "canvas: world rect width scales with zoom",
          "width=" + std::to_string(zoomed_width));
    check(near(fixed_width, 10.0f, 0.01f), "canvas: screen rect ignores zoom",
          "width=" + std::to_string(fixed_width));
}

void test_canvas_circle_segments()
{
    gdemo::Canvas small;
    gdemo::Camera camera = centered();
    camera.zoom = 0.1f;
    small.begin(nullptr, camera, {800.0f, 600.0f});
    small.circle({0.0f, 0.0f}, 10.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    const std::size_t small_vertices = small.vertex_count();

    gdemo::Canvas large;
    camera.zoom = 8.0f;
    large.begin(nullptr, camera, {800.0f, 600.0f});
    large.circle({0.0f, 0.0f}, 10.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    const std::size_t large_vertices = large.vertex_count();

    check(small_vertices >= 13, "canvas: tiny circle keeps a minimum segment count",
          "verts=" + std::to_string(small_vertices));
    check(large_vertices > small_vertices, "canvas: bigger on screen => more circle segments",
          std::to_string(small_vertices) + " -> " + std::to_string(large_vertices));
}

void test_canvas_outline_bounds()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered();
    canvas.begin(nullptr, camera, {800.0f, 600.0f});
    canvas.outline({0.0f, 0.0f}, {100.0f, 50.0f}, 6.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    const auto vertices = flatten(canvas);
    check(vertices.size() == 16, "canvas: outline emits four edge quads",
          "verts=" + std::to_string(vertices.size()));

    // 四条边都必须落在矩形包围盒内（不得溢出、不得歪斜）。
    float min_x = 1e9f, min_y = 1e9f, max_x = -1e9f, max_y = -1e9f;
    for (const auto& vertex : vertices) {
        min_x = std::min(min_x, vertex.pos.x);
        min_y = std::min(min_y, vertex.pos.y);
        max_x = std::max(max_x, vertex.pos.x);
        max_y = std::max(max_y, vertex.pos.y);
    }
    // 边都是轴对齐矩形：x 只能是 400 / 406 / 494 / 500，y 同理。
    bool axis_aligned = true;
    for (const auto& vertex : vertices) {
        const float x = vertex.pos.x;
        const float y = vertex.pos.y;
        const bool x_ok = near(x, 400.0f, 0.01f) || near(x, 406.0f, 0.01f) ||
                          near(x, 494.0f, 0.01f) || near(x, 500.0f, 0.01f);
        const bool y_ok = near(y, 300.0f, 0.01f) || near(y, 306.0f, 0.01f) ||
                          near(y, 344.0f, 0.01f) || near(y, 350.0f, 0.01f);
        if (!x_ok || !y_ok) {
            axis_aligned = false;
            std::cout << "  outline vertex (" << x << ", " << y << ")\n";
        }
    }
    check(axis_aligned, "canvas: outline edges are axis-aligned quads");

    // 每个边 quad 必须是 4 个不同的角点、x/y 各取两个值（否则就是斜边/退化三角形）。
    bool quads_are_rects = true;
    for (std::size_t q = 0; q + 3 < vertices.size(); q += 4) {
        std::vector<glm::vec2> corners;
        for (std::size_t k = 0; k < 4; ++k) {
            const glm::vec2 p = vertices[q + k].pos;
            bool seen = false;
            for (const auto& other : corners) {
                if (near(p, other, 0.01f)) seen = true;
            }
            if (!seen) corners.push_back(p);
        }
        if (corners.size() != 4) {
            quads_are_rects = false;
            continue;
        }
        int distinct_x = 0;
        int distinct_y = 0;
        for (std::size_t i = 0; i < corners.size(); ++i) {
            bool new_x = true;
            bool new_y = true;
            for (std::size_t j = 0; j < i; ++j) {
                if (near(corners[i].x, corners[j].x, 0.01f)) new_x = false;
                if (near(corners[i].y, corners[j].y, 0.01f)) new_y = false;
            }
            if (new_x) ++distinct_x;
            if (new_y) ++distinct_y;
        }
        if (distinct_x != 2 || distinct_y != 2) quads_are_rects = false;
    }
    check(quads_are_rects, "canvas: each outline edge is a proper 4-corner quad");
    check(near(min_x, 400.0f, 0.01f) && near(min_y, 300.0f, 0.01f),
          "canvas: outline starts at the rect origin",
          std::to_string(min_x) + "," + std::to_string(min_y));
    check(near(max_x, 500.0f, 0.01f) && near(max_y, 350.0f, 0.01f),
          "canvas: outline ends at the rect corner",
          std::to_string(max_x) + "," + std::to_string(max_y));
}

void test_canvas_polyline_and_grid()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered();
    canvas.begin(nullptr, camera, {800.0f, 600.0f});
    const std::vector<glm::vec2> points{{0.0f, 0.0f}, {10.0f, 10.0f}, {20.0f, 0.0f}};
    canvas.polyline(points, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    check(canvas.vertex_count() == 8, "canvas: polyline emits one quad per segment",
          "verts=" + std::to_string(canvas.vertex_count()));

    gdemo::Canvas empty;
    empty.begin(nullptr, camera, {800.0f, 600.0f});
    empty.grid({-100.0f, -100.0f}, {100.0f, 100.0f}, 100.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    check(empty.vertex_count() > 0, "canvas: grid emits lines when visible");

    gdemo::Canvas dense;
    gdemo::Camera tiny = centered();
    tiny.zoom = 0.001f;
    dense.begin(nullptr, tiny, {800.0f, 600.0f});
    dense.grid({-100.0f, -100.0f}, {100.0f, 100.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    check(dense.vertex_count() == 0, "canvas: grid skips steps that would be sub-pixel");
}

void test_canvas_text_without_atlas()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered();
    canvas.begin(nullptr, camera, {800.0f, 600.0f});
    canvas.text({0.0f, 0.0f}, 16.0f, "hello", {1.0f, 1.0f, 1.0f, 1.0f});
    canvas.screen_text({0.0f, 0.0f}, 16.0f, "hud", {1.0f, 1.0f, 1.0f, 1.0f});
    check(canvas.vertex_count() == 0, "canvas: text without an atlas is a no-op");
    check(near(canvas.text_width("abc", 10.0f), 15.0f, 1.0f),
          "canvas: text_width falls back to the estimate without an atlas");
}

void test_canvas_append_to_deep_copies()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered();
    canvas.begin(nullptr, camera, {800.0f, 600.0f});
    canvas.rect({0.0f, 0.0f}, {10.0f, 10.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
    canvas.rect({20.0f, 0.0f}, {10.0f, 10.0f}, {1.0f, 1.0f, 1.0f, 1.0f});

    xel::graphics::DrawList target;
    target.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    canvas.append_to(target);

    std::size_t vertices = 0;
    std::size_t indices = 0;
    for (const auto& batch : target.batches()) {
        vertices += batch.vertices.size();
        indices += batch.indices.size();
    }
    check(vertices == canvas.vertex_count(), "canvas: append_to copies every vertex",
          std::to_string(vertices) + " vs " + std::to_string(canvas.vertex_count()));
    check(indices == canvas.index_count(), "canvas: append_to copies every index");

    // 索引必须整体平移到目标 batch 的顶点基址，否则会画出别的几何。
    bool indices_in_range = true;
    for (const auto& batch : target.batches()) {
        for (const uint32_t index : batch.indices) {
            if (index >= batch.vertices.size()) indices_in_range = false;
        }
    }
    check(indices_in_range, "canvas: append_to offsets indices into the destination batch");
}

// ── 场景 ──────────────────────────────────────────────────────────
void test_scene_registry()
{
    const auto& list = gdemo::pages();
    check(!list.empty(), "scene: registry is not empty");
    std::set<std::string> ids;
    int incomplete = 0;
    for (const auto& page : list) {
        if (page.id.empty() || page.title.empty() || page.build == nullptr) ++incomplete;
        ids.insert(std::string{page.id});
    }
    check(incomplete == 0, "scene: every page has id/title/build",
          "incomplete=" + std::to_string(incomplete));
    check(ids.size() == list.size(), "scene: page ids are unique");
    check(gdemo::page_index("curves") >= 0, "scene: page_index finds a known id");
    check(gdemo::page_index("nope") == -1, "scene: page_index returns -1 for unknown ids");
}

void test_scene_pages_draw_something()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered({1280.0f, 840.0f});
    camera.fit({1280.0f, 840.0f}, gdemo::kContentSize);

    gdemo::SceneOptions options;
    const auto& list = gdemo::pages();
    for (std::size_t i = 0; i < list.size(); ++i) {
        options.page = static_cast<int>(i);
        canvas.begin(nullptr, camera, {1280.0f, 840.0f});
        gdemo::build_scene(canvas, options, 0.0f);
        check(canvas.vertex_count() > 0, "scene: page draws geometry",
              std::string{list[i].id} + " verts=" + std::to_string(canvas.vertex_count()));
    }
}

void test_scene_grid_toggle()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered({1280.0f, 840.0f});
    camera.fit({1280.0f, 840.0f}, gdemo::kContentSize);

    gdemo::SceneOptions options;
    options.page = gdemo::page_index("batching");
    options.density = 8;
    options.grid = false;
    options.axes = false;
    canvas.begin(nullptr, camera, {1280.0f, 840.0f});
    gdemo::build_scene(canvas, options, 0.0f);
    const std::size_t plain = canvas.vertex_count();

    options.grid = true;
    options.axes = true;
    canvas.begin(nullptr, camera, {1280.0f, 840.0f});
    gdemo::build_scene(canvas, options, 0.0f);
    check(canvas.vertex_count() > plain, "scene: grid/axes add geometry",
          std::to_string(plain) + " -> " + std::to_string(canvas.vertex_count()));
}

void test_scene_density_grows_vertices()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered({1280.0f, 840.0f});
    camera.fit({1280.0f, 840.0f}, gdemo::kContentSize);

    gdemo::SceneOptions options;
    options.page = gdemo::page_index("batching");
    options.grid = false;
    options.axes = false;

    options.density = 4;
    canvas.begin(nullptr, camera, {1280.0f, 840.0f});
    gdemo::build_scene(canvas, options, 0.0f);
    const std::size_t sparse = canvas.vertex_count();
    const std::size_t sparse_batches = canvas.batch_count();

    options.density = 16;
    canvas.begin(nullptr, camera, {1280.0f, 840.0f});
    gdemo::build_scene(canvas, options, 0.0f);
    check(canvas.vertex_count() > sparse, "scene: higher density => more vertices",
          std::to_string(sparse) + " -> " + std::to_string(canvas.vertex_count()));
    check(canvas.batch_count() == sparse_batches,
          "scene: density does not add draw calls (same slot/clip)",
          std::to_string(sparse_batches) + " -> " + std::to_string(canvas.batch_count()));
}

void test_scene_hud()
{
    gdemo::Canvas canvas;
    gdemo::Camera camera = centered({1280.0f, 840.0f});
    canvas.begin(nullptr, camera, {1280.0f, 840.0f});
    gdemo::SceneOptions options;
    gdemo::HudInfo info;
    info.fps = 60.0f;
    gdemo::build_hud(canvas, options, info);
    // 无 atlas 时文字不画，但 HUD 的底板矩形必须还在（否则导出图会缺条）。
    check(canvas.vertex_count() >= 8, "scene: hud draws panels even without a font atlas",
          "verts=" + std::to_string(canvas.vertex_count()));
}

// ── 分层：这个示例不得依赖 UI ─────────────────────────────────────
void test_dependency_boundary()
{
    const fs::path root = fs::path(COSINE_SOURCE_DIR) / "examples" / "graphics_demo";
    check(fs::exists(root), "boundary: graphics_demo source directory exists");

    std::vector<fs::path> sources;
    for (const auto& entry : fs::directory_iterator(root)) {
        if (entry.path().extension() == ".cpp" || entry.path().extension() == ".hpp") {
            sources.push_back(entry.path());
        }
    }
    check(!sources.empty(), "boundary: graphics_demo has sources");

    int ui_includes = 0;
    int backend_includes = 0;
    for (const auto& path : sources) {
        std::istringstream in(read_file(path));
        std::string line;
        while (std::getline(in, line)) {
            if (line.find("#include") == std::string::npos) continue;
            if (line.find("\"ui/") != std::string::npos) ++ui_includes;
            if (line.find("\"backend/vulkan") != std::string::npos) ++backend_includes;
        }
    }
    check(ui_includes == 0, "boundary: graphics_demo does not include ui/*",
          "includes=" + std::to_string(ui_includes));
    check(backend_includes == 0, "boundary: graphics_demo does not include backend/vulkan/*",
          "includes=" + std::to_string(backend_includes));
}
} // namespace

int main()
{
    test_camera_roundtrip();
    test_camera_zoom_anchor();
    test_camera_zoom_clamp();
    test_camera_fit();
    test_camera_pan();

    test_canvas_rect();
    test_canvas_batches_merge();
    test_canvas_screen_space_is_not_zoomed();
    test_canvas_circle_segments();
    test_canvas_outline_bounds();
    test_canvas_polyline_and_grid();
    test_canvas_text_without_atlas();
    test_canvas_append_to_deep_copies();

    test_scene_registry();
    test_scene_pages_draw_something();
    test_scene_grid_toggle();
    test_scene_density_grows_vertices();
    test_scene_hud();

    test_dependency_boundary();

    if (failures != 0) {
        std::cout << "graphics demo: " << failures << " failed\n";
        return 1;
    }
    std::cout << "graphics demo: all checks passed\n";
    return 0;
}
