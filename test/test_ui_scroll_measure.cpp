// ScrollView 在父容器中的尺寸契约回归。
// 历史 bug：measure() 返回 viewport_（已扣滚动条），父级按窄尺寸摆放，
// arrange() 里又扣一次 → 控件变窄 + 内容区扣两次。
#include "ui/containers/scroll_view.hpp"
#include "ui/containers/vbox.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/system/root.hpp"
#include "ui/style/theme.hpp"
#include "ui/system/ui_system.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <memory>

using namespace xel;

namespace
{
bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

constexpr float kOuterW = 300.0f;
constexpr float kOuterH = 200.0f;

// 构建一个 Vbox（默认 cross_align = top_center，不拉伸）包住 ScrollView，
// 内容是一个固定宽高的 ColorRect。
struct Fixture
{
    std::unique_ptr<ui::UISystem> system;
    ui::UIRoot* root = nullptr;
    ui::ScrollView* scroll = nullptr;

    explicit Fixture(float content_h)
    {
        auto r = std::make_unique<ui::UIRoot>(0.0f, 0.0f, kOuterW, kOuterH);
        root = r.get();
        system = std::make_unique<ui::UISystem>(std::move(r));

        auto box = std::make_unique<ui::Vbox>(0.0f, 0.0f, kOuterW, kOuterH);
        box->set_padding(0.0f);
        // 默认 cross_align = top_center：非 stretch，会暴露 measure 契约错误。
        auto view = std::make_unique<ui::ScrollView>();
        scroll = view.get();
        view->set_content(std::make_unique<ui::ColorRect>(0.0f, 0.0f, kOuterW, content_h,
                                                          glm::vec4{1.0f}));
        box->add(std::move(view));
        root->add(std::move(box));
    }

    void tick() { system->tick(); }
};

void test_no_scrollbar_takes_full_width()
{
    Fixture f(/*content_h=*/50.0f); // 内容比视口矮 → 无滚动条
    f.tick();

    assert(!f.scroll->scrollbar_visible(true));
    assert(near(f.scroll->bounds().size.x, kOuterW));
    assert(near(f.scroll->viewport_size().x, kOuterW));
    std::printf("no scrollbar: full width ok\n");
}

void test_scrollbar_keeps_widget_width()
{
    Fixture f(/*content_h=*/400.0f); // 内容超高 → 出现垂直滚动条
    f.tick();

    assert(f.scroll->scrollbar_visible(true));

    const float bar = ui::Theme{}.scrollbar_width + 2.0f;

    // 控件自身仍是父给的可用宽度（不被滚动条挤窄）。
    assert(near(f.scroll->bounds().size.x, kOuterW));
    // 内容可视区只扣一次滚动条宽度。
    assert(near(f.scroll->viewport_size().x, kOuterW - bar));
    std::printf("scrollbar: widget width kept, viewport deducted once ok\n");
}

void test_width_stable_across_scrollbar_toggle()
{
    // 同一 ScrollView 从「内容矮」变「内容高」，控件宽度不应变化。
    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, kOuterW, kOuterH);
    ui::UISystem system{std::move(root)};

    auto box = std::make_unique<ui::Vbox>(0.0f, 0.0f, kOuterW, kOuterH);
    box->set_padding(0.0f);
    auto color = std::make_unique<ui::ColorRect>(0.0f, 0.0f, kOuterW, 50.0f, glm::vec4{1.0f});
    ui::ColorRect* color_ptr = color.get();
    auto view = std::make_unique<ui::ScrollView>();
    ui::ScrollView* view_ptr = view.get();
    view->set_content(std::move(color));
    box->add(std::move(view));
    system.root()->add(std::move(box));
    system.tick();

    const float before = view_ptr->bounds().size.x;
    assert(near(before, kOuterW));
    assert(!view_ptr->scrollbar_visible(true));

    // 内容变高 → 滚动条出现。
    // UI-32 后 set_size 只写几何；内容自然尺寸要用 request_size（ScrollView 靠 measure 算内容高）。
    color_ptr->request_size({kOuterW, 400.0f});
    view_ptr->mark_layout_dirty();
    system.tick();

    assert(view_ptr->scrollbar_visible(true));
    assert(near(view_ptr->bounds().size.x, before)); // 宽度不变
    std::printf("width stable across scrollbar toggle ok\n");
}
} // namespace

int main()
{
    test_no_scrollbar_takes_full_width();
    test_scrollbar_keeps_widget_width();
    test_width_stable_across_scrollbar_toggle();
    std::printf("scroll view measure tests passed\n");
    return 0;
}
