// 浮层与模态 / 媒体与时间 两个分区。
#include "sections/sections.hpp"

#include "ui/composites/dialog.hpp"
#include "ui/composites/dropdown.hpp"
#include "ui/composites/window.hpp"
#include "ui/containers/grid.hpp"
#include "ui/core/button.hpp"
#include "ui/core/color_rect.hpp"
#include "ui/core/image.hpp"
#include "ui/core/label.hpp"
#include "ui/core/preview_ruler.hpp"
#include "ui/core/tag.hpp"
#include "ui/core/tooltip.hpp"
#include "ui/core/typography.hpp"

#include <memory>
#include <string>
#include <vector>

namespace demo::sections
{
void build_overlays(Demo& demo, ui::Vbox& column)
{
    auto& layers = group(column, "层叠：Content / Floating / Modal");
    hint(layers,
         "浮层是 root 的普通子项，靠 Layer 决定叠放："
         "Tooltip、Dropdown 列表属 Floating；对话框与遮罩属 Modal。"
         "layer 压过 z_index，且不受面板裁剪。");

    auto& window_row = row(layers);
    auto open_window = std::make_unique<ui::Button>("打开窗口 Window");
    auto& root = demo.root();
    open_window->set_on_click([&root, &demo] {
        auto window = std::make_unique<ui::Window>("演示窗口", glm::vec2{120.0f, 120.0f},
                                                  glm::vec2{320.0f, 220.0f});
        window->add(std::make_unique<ui::Paragraph>(
            "拖动标题栏移动、拖边缘改尺寸、双击标题栏最大化、点关闭按钮销毁。"));
        window->add(std::make_unique<ui::Label>("窗口内容：Label"));
        window->set_footer(std::make_unique<ui::Label>("footer：可放状态/按钮"));
        root.add(std::move(window));
        demo.note("已打开一个 Window（拖动/缩放/关闭）");
    });
    window_row.add(std::move(open_window));

    auto open_modal = std::make_unique<ui::Button>("打开模态 Window");
    open_modal->set_on_click([&root, &demo] {
        auto window = std::make_unique<ui::Window>("模态窗口", glm::vec2{200.0f, 160.0f},
                                                  glm::vec2{340.0f, 200.0f});
        window->add(std::make_unique<ui::Paragraph>(
            "模态：全屏遮罩挡住下层，Esc 可关闭；关闭后自动恢复之前的键盘焦点。"));
        window->set_modal(true);
        root.add(std::move(window));
        demo.note("已打开模态窗口：Esc 关闭");
    });
    window_row.add(std::move(open_modal));

    auto& dialogs = group(column, "对话框 Dialog");
    hint(dialogs, "Dialog 是「标题 + 消息 + 动作按钮」的成品，宿主自动进入模态作用域。");
    auto& dialog_row = row(dialogs);
    auto ask = std::make_unique<ui::Button>("确认对话框");
    ask->set_on_click([&root, &demo] {
        auto dialog = std::make_unique<ui::Dialog>("删除资源？", 360.0f);
        dialog->set_message("这个操作不可撤销。确定要删除 3 个选中资源吗？");
        dialog->add_action("取消", [] {}, false);
        dialog->add_action("删除", [&demo] { demo.note("对话框：已删除（演示，无实际动作）"); },
                           true);
        ui::show_dialog(root, std::move(dialog),
                        [&demo] { demo.note("对话框已关闭"); });
        demo.note("对话框已打开：点按钮或 Esc 关闭");
    });
    dialog_row.add(std::move(ask));

    auto& dropdown_note = row(dialogs);
    dropdown_note.add(std::make_unique<ui::Paragraph>(
        "下拉与提示在这套层叠规则下工作：列表挂在 root（Floating），所以不会被滚动面板裁掉。"));

    auto& tooltip_group = group(column, "Tooltip 与悬停信息");
    hint(tooltip_group,
         "Tooltip 由 UISystem 的悬停目标驱动：延迟出现、跟随指针、离开即隐藏。");
    auto& tooltip_row = row(tooltip_group);
    tooltip_row.add(std::make_unique<ui::Tag>("悬停我"));
    tooltip_row.add(std::make_unique<ui::Button>("按钮也能悬停",
        [&demo] { demo.note("按钮点击：Tooltip 分区"); }));

    auto& menu = group(column, "菜单 MenuBar");
    hint(menu,
         "顶栏那条就是 MenuBar：点开菜单后子菜单支持左右进入/退出、Esc 全关。");
    menu.add(std::make_unique<ui::Paragraph>(
        "除了顶栏的三个菜单，其余控件都在内容区。菜单弹出层与其他浮层共用同一套 Layer 规则。"));
}

void build_media(Demo& demo, ui::Vbox& column)
{
    auto& image = group(column, "Image：占位与纹理");
    hint(image,
         "texture_id == 0 时画占位（圆角底 + 边框 + 对角线 + 文案）；"
         "填写纹理槽后按 Fit 规则（Fill / Contain / Cover）摆放 UV。");
    auto& image_row = row(image);
    auto placeholder = std::make_unique<ui::Image>(160.0f, 96.0f);
    placeholder->set_background_visible(true);
    placeholder->set_border_visible(true);
    image_row.add(std::move(placeholder));

    auto fit_contain = std::make_unique<ui::Image>(120.0f, 96.0f);
    fit_contain->set_fit(ui::Image::Fit::Contain);
    fit_contain->set_background_visible(true);
    image_row.add(std::move(fit_contain));

    auto fit_cover = std::make_unique<ui::Image>(120.0f, 96.0f);
    fit_cover->set_fit(ui::Image::Fit::Cover);
    fit_cover->set_background_visible(true);
    image_row.add(std::move(fit_cover));
    image_row.add(std::make_unique<ui::Label>("Placeholder / Contain / Cover"));

    auto& ruler = group(column, "PreviewRuler：拖动定位");
    hint(ruler, "点击或拖动改变时间；回调里把值写回标签（时间轴头部的同款控件）。");
    auto& ruler_row = row(ruler);
    auto view = std::make_unique<ui::PreviewRuler>();
    view->set_duration(10.0f);
    view->set_time(3.0f);
    view->set_fill(true, false);
    auto* ruler_ptr = &*view;
    auto echo = std::make_unique<ui::Label>("time = 3.00 s");
    auto* echo_ptr = echo.get();
    view->set_on_seek([echo_ptr](float seconds) {
        char text[64];
        std::snprintf(text, sizeof(text), "time = %.2f s", static_cast<double>(seconds));
        echo_ptr->set_text(text);
    });
    ruler_row.add(std::move(view));
    ruler_row.add(std::move(echo));
    (void)ruler_ptr;

    auto& palette = group(column, "色板与主题色");
    hint(palette, "这些色块用当前主题的强调色与背景色，切主题时会跟着变。");
    auto swatches = std::make_unique<ui::Grid>(0.0f, 0.0f, 0.0f, 0.0f);
    swatches->set_columns(8);
    swatches->set_spacing(6.0f, 6.0f);
    swatches->set_fill(true, false);
    for (int i = 0; i < 8; ++i) {
        const float t = static_cast<float>(i) / 7.0f;
        swatches->add(std::make_unique<ui::ColorRect>(
            0.0f, 0.0f, 0.0f, 26.0f, glm::vec4{0.24f + t * 0.5f, 0.56f - t * 0.3f, 0.91f - t * 0.4f, 1.0f}));
    }
    palette.add(std::move(swatches));

    demo.note("媒体分区：Image 占位、PreviewRuler 拖动、色板随主题变化");
}
} // namespace demo::sections
