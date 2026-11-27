// UI-51：测量缓存统一的契约测试。
//
// 改造前三套缓存（Widget 的 constraints+epoch / Label 的文本键 / TreeView 的可见行），
// 现在统一成 MeasureCache + 可扩展 MeasureKey：
//   1. 键语义：布局级输入（constraints/epoch）+ 派生输入（文本哈希/字号/换行…）；
//   2. 单一存储：invalidate_measure() 能清掉派生类（Label）的缓存；
//   3. 控件的派生输入变化（改文本）必须 miss，相同输入必须 hit；
//   4. TreeView 的行缓存挂在同一失效路径上。
#include "ui/core/label.hpp"
#include "ui/measure_cache.hpp"
#include "ui/theme.hpp"
#include "ui/ui_system.hpp"
#include "ui/views/tree_view.hpp"
#include "ui/widget.hpp"

#include "graphics/font_atlas.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace xel;

namespace
{
int g_passed = 0;
int g_failed = 0;

void check(bool ok, const char* name, const std::string& detail = std::string())
{
    if (ok) {
        ++g_passed;
        std::printf("ok      %s\n", name);
        return;
    }
    ++g_failed;
    std::printf("FAIL    %s -- %s\n", name, detail.c_str());
}

// 暴露受保护的缓存接口，用于断言命中语义。
class CacheProbe : public ui::Widget
{
public:
    glm::vec2 measure(glm::vec2) override { return {10.0f, 10.0f}; }
    void arrange(ui::Rect slot) override { set_geometry(slot); }
    void append_to(graphics::DrawList&) const override {}

    bool hit(const ui::MeasureKey& key) const { return measure_cache_hit(key); }
    void store(const ui::MeasureKey& key, glm::vec2 value) { store_measure(key, value); }
    ui::MeasureKey base_key(glm::vec2 constraints) const
    {
        ui::MeasureKey key;
        key.constraints = constraints;
        key.epoch = ui::layout_epoch();
        return key;
    }
};

class LabelProbe : public ui::Label
{
public:
    LabelProbe() : ui::Label("alpha", 16.0f) {}
    bool hit_text(std::string_view text, float size)
    {
        ui::MeasureKey key;
        key.epoch = ui::layout_epoch();
        key.text_hash = ui::measure_hash_text(text);
        key.font_size = size;
        key.wrap_width = -1.0f;
        key.resource = font_atlas();
        return measure_cache_hit(key);
    }
};

// 最小图集：Label 只有拿到 FontAtlas 才会走带缓存的测量路径。
struct AtlasRig
{
    graphics::FontAtlas atlas;
    AtlasRig()
    {
        std::vector<unsigned char> pixels(64 * 64 * 4, 0);
        atlas.load(64, 64, pixels.data());
        std::vector<unsigned char> bitmap(8 * 10, 200);
        atlas.add_glyph(U'a', bitmap.data(), 8, 10, 8, 0.6f, 0.1f, 0.7f, 16.0f);
        graphics::set_active_font_atlas(&atlas);
    }
    ~AtlasRig() { graphics::set_active_font_atlas(nullptr); }
};
} // namespace

// 1) 键语义：同一轮 + 同约束 + 同派生输入 = hit；任何一项变 = miss。
void test_key_semantics()
{
    CacheProbe probe;
    const glm::vec2 constraints{120.0f, 40.0f};
    const auto key = probe.base_key(constraints);

    probe.store(key, {100.0f, 30.0f});
    check(probe.hit(key), "same key hits");
    check(probe.measure_valid(), "store marks the cache valid");

    auto other = key;
    other.constraints = {200.0f, 40.0f};
    check(!probe.hit(other), "different constraints miss");

    auto next_epoch = key;
    next_epoch.epoch += 1;
    check(!probe.hit(next_epoch), "different epoch misses");

    auto with_text = key;
    with_text.text_hash = ui::measure_hash_text("hello");
    check(!probe.hit(with_text), "different derived input misses");

    check(ui::measure_hash_text("abc") != ui::measure_hash_text("abd"),
          "text hash distinguishes different strings");
    check(ui::measure_hash_text("abc") == ui::measure_hash_text("abc"), "text hash is stable");
}

// 2) 单一存储：invalidate_measure() 清掉的是同一份缓存。
void test_single_storage()
{
    CacheProbe probe;
    const auto key = probe.base_key({50.0f, 50.0f});
    probe.store(key, {40.0f, 40.0f});
    check(probe.hit(key), "cache is populated");

    probe.invalidate_measure();
    check(!probe.measure_valid(), "invalidate_measure clears the unified cache");
    check(!probe.hit(key), "cleared cache no longer hits");
}

// 3) Label：用同一份缓存，改文本必须 miss，同文本必须 hit。
void test_label_uses_unified_cache()
{
    AtlasRig rig;
    LabelProbe label;
    label.measure({300.0f, 200.0f});
    check(label.hit_text("alpha", label.text_size()),
          "Label caches its measurement in the unified cache");

    label.set_text("changed");
    check(!label.hit_text("alpha", label.text_size()),
          "changing the text invalidates the hit (key differs)");
    label.measure({300.0f, 200.0f});
    check(label.hit_text("changed", label.text_size()), "re-measured with the new text");

    label.invalidate_measure();
    check(!label.measure_valid(), "invalidate_measure clears Label's cache too");

    // 字号变化（主题字号或显式 set_text_size）也必须 miss。
    label.measure({300.0f, 200.0f});
    check(label.hit_text("changed", label.text_size()), "re-populated after invalidate");
    label.set_text_size(label.text_size() + 4.0f);
    check(!label.hit_text("changed", label.text_size() - 4.0f),
          "font size change misses (theme text_size path)");
}

// 4) TreeView：行缓存挂在同一失效路径上（invalidate_measure 也丢行缓存）。
void test_tree_view_shares_invalidation()
{
    auto make_node = [](std::string name) {
        ui::TreeNode node;
        node.title = std::move(name);
        return node;
    };

    ui::TreeView tree;
    std::vector<ui::TreeNode> roots;
    roots.push_back(make_node("root"));
    roots.back().children.push_back(make_node("child"));
    tree.set_roots(std::move(roots));

    const std::size_t before = tree.visible_count();
    check(before >= 1, "tree reports visible rows");

    tree.invalidate_measure();
    check(!tree.measure_valid(), "invalidate_measure clears the tree's measure cache");
    check(tree.visible_count() == before, "visible rows are recomputed to the same count");
}

// 5) 缓存不再让控件体积失控（MeasureKey 内不放 std::string）。
void test_cache_key_is_compact()
{
    static_assert(sizeof(ui::MeasureKey) <= 64, "MeasureKey 必须 ≤64 B（文本用哈希而非 std::string）");
    static_assert(sizeof(ui::MeasureCache) <= 96, "MeasureCache 必须 ≤96 B");
    check(sizeof(ui::MeasureKey) <= 64, "MeasureKey stays under 64 bytes");
    check(sizeof(ui::Widget) <= 440, "Widget stays under 440 bytes with the unified cache");
}

int main()
{
    test_key_semantics();
    test_single_storage();
    test_label_uses_unified_cache();
    test_tree_view_shares_invalidation();
    test_cache_key_is_compact();

    std::printf("measure cache: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
