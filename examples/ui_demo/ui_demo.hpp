#pragma once

#include "graphics/text_debug.hpp"
#include "ui/color_rect.hpp"
#include "ui/debug_overlay.hpp"
#include "ui/hbox.hpp"
#include "ui/label.hpp"
#include "ui/panel.hpp"
#include "ui/root.hpp"
#include "ui/scroll_view.hpp"
#include "ui/typography.hpp"
#include "ui/vbox.hpp"

#include <cstdlib>
#include <format>
#include <limits>
#include <string_view>
#include <memory>
#include <string>
#include <vector>

namespace xel::demo
{
// 文字实验台：只放文本样例，用快捷键切换调试层与排版选项。
struct TextLab
{
    graphics::TextDebugOptions options{};
    bool pixel_snap = true;
    std::vector<ui::Label*> samples;
    ui::Label* info = nullptr;
    ui::DebugOverlay* overlay = nullptr;

    void refresh()
    {
        for (auto* label : samples) {
            label->set_text_debug(options);
            label->set_pixel_snap(pixel_snap);
        }
        if (info) {
            info->set_text(std::format(
                "glyphs: {}   line box: {}   baseline: {}   advance: {}   snap: {}",
                options.glyph_boxes ? "on" : "off",
                options.line_box ? "on" : "off",
                options.baseline ? "on" : "off",
                options.advance ? "on" : "off",
                pixel_snap ? "on" : "off"));
        }
    }

    void toggle_glyph_boxes() { options.glyph_boxes = !options.glyph_boxes; refresh(); }
    void toggle_line_box()
    {
        options.line_box = !options.line_box;
        options.baseline = options.line_box;
        refresh();
    }
    void toggle_advance() { options.advance = !options.advance; refresh(); }
    void toggle_pixel_snap() { pixel_snap = !pixel_snap; refresh(); }
};

inline TextLab build(ui::UIRoot& root)
{
    TextLab lab;

    auto panel = std::make_unique<ui::Panel>(24.0f, 24.0f, 1152.0f, 752.0f);
    panel->set_min_size({1152.0f, 752.0f});

    auto content = std::make_unique<ui::Vbox>(0.0f, 0.0f, 1104.0f, 720.0f);
    content->set_spacing(12.0f);
    content->set_cross_align(ui::Align::top_left());

    auto add_sample = [&](std::unique_ptr<ui::Label> label) {
        lab.samples.push_back(label.get());
        content->add(std::move(label));
    };

    auto title = std::make_unique<ui::Heading>("Text Display Lab", 1);
    add_sample(std::move(title));

    auto hint = std::make_unique<ui::Label>(
        "F1 overlay   F2 glyph boxes   F3 line box + baseline   F4 advance   F5 pixel snap",
        13.0f);
    add_sample(std::move(hint));

    content->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 1104.0f, 1.0f,
                                                 glm::vec4{1.0f, 1.0f, 1.0f, 0.12f}));

    content->add(std::make_unique<ui::Heading>("Roles", 2));
    struct RoleSample { ui::TextRole role; const char* text; };
    const RoleSample roles[] = {
        {ui::TextRole::Display, "Display jqpy 128"},
        {ui::TextRole::Title, "Title  一段标题"},
        {ui::TextRole::Heading, "Heading  二级标题"},
        {ui::TextRole::Subheading, "Subheading jqpy 三级标题"},
        {ui::TextRole::Body, "Body  正文 Hello, world! 你好，世界。 0123456789"},
        {ui::TextRole::Caption, "Caption  辅助说明文字"},
    };
    for (const auto& sample : roles) {
        auto label = std::make_unique<ui::Label>(sample.text);
        ui::apply_text_role(*label, sample.role);
        add_sample(std::move(label));
    }

    content->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 1104.0f, 1.0f,
                                                 glm::vec4{1.0f, 1.0f, 1.0f, 0.12f}));

    content->add(std::make_unique<ui::Heading>("Alignment", 2));
    const std::pair<ui::TextAlign, const char*> aligns[] = {
        {ui::TextAlign::Left, "Left aligned text"},
        {ui::TextAlign::Center, "Centered text"},
        {ui::TextAlign::Right, "Right aligned text"},
    };
    for (const auto& [align, text] : aligns) {
        auto label = std::make_unique<ui::Label>(text);
        label->set_text_align(align);
        label->set_min_size({560.0f, 0.0f});
        add_sample(std::move(label));
    }

    content->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 1104.0f, 1.0f,
                                                 glm::vec4{1.0f, 1.0f, 1.0f, 0.12f}));

    content->add(std::make_unique<ui::Heading>("Wrapping & line spacing", 2));
    auto paragraph = std::make_unique<ui::Paragraph>(
        "Wrapping keeps long sentences inside the available width. 这一行用于检查中文换行与行距。"
        "Line spacing is configurable per label, and the measure pass stays consistent with drawing.");
    paragraph->set_min_size({720.0f, 0.0f});
    paragraph->set_max_size({720.0f, std::numeric_limits<float>::max()});
    paragraph->set_line_spacing(6.0f);
    add_sample(std::move(paragraph));

    auto paragraph_tight = std::make_unique<ui::Paragraph>(
        "Same paragraph with 0px extra line spacing for comparison.");
    paragraph_tight->set_min_size({720.0f, 0.0f});
    paragraph_tight->set_max_size({720.0f, std::numeric_limits<float>::max()});
    add_sample(std::move(paragraph_tight));

    content->add(std::make_unique<ui::ColorRect>(0.0f, 0.0f, 1104.0f, 1.0f,
                                                 glm::vec4{1.0f, 1.0f, 1.0f, 0.12f}));

    content->add(std::make_unique<ui::Heading>("Descenders at large size", 2));
    auto big = std::make_unique<ui::Label>("Hqyp gjpqy", 96.0f);
    add_sample(std::move(big));

    content->add(std::make_unique<ui::Heading>("CJK & dynamic glyphs", 2));
    add_sample(std::make_unique<ui::Label>("中文排版：你好，世界！动态字形：囧 鑫 龘"));
    add_sample(std::make_unique<ui::Label>("Mixed: Latin 中文 0123 — punctuation ，。！"));

    auto info = std::make_unique<ui::Label>("", 13.0f);
    lab.info = info.get();
    content->add(std::move(info));

    panel->add(std::move(content));
    root.add(std::move(panel));

    auto overlay = std::make_unique<ui::DebugOverlay>();
    lab.overlay = overlay.get();
    root.add_overlay(std::move(overlay));

    if (std::getenv("COSINE_TEXT_DEBUG")) {
        lab.options.glyph_boxes = true;
        lab.options.line_box = true;
        lab.options.baseline = true;
        lab.options.advance = true;
    }
    if (const char* snap = std::getenv("COSINE_TEXT_SNAP")) lab.pixel_snap = std::string_view{snap} != "0";

    lab.refresh();
    return lab;
}
} // namespace xel::demo
