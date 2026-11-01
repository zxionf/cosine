#include "ui/theme.hpp"

namespace xel::ui
{
Theme Theme::with_density(Density target) const
{
    Theme theme_value = *this;
    const float metric = density_metric_factor(target) / density_metric_factor(density);
    const float text_scale = density_text_factor(target) / density_text_factor(density);
    theme_value.density = target;

    theme_value.padding *= metric;
    theme_value.spacing *= metric;
    theme_value.control_height *= metric;
    theme_value.row_height *= metric;
    theme_value.checkbox_size *= metric;
    theme_value.slider_height *= metric;
    theme_value.slider_handle_radius *= metric;
    theme_value.scrollbar_width *= metric;
    theme_value.scrollbar_min_thumb *= metric;
    theme_value.inset_shadow_size *= metric;

    theme_value.text_size *= text_scale;
    theme_value.small_text_size *= text_scale;
    theme_value.heading_text_size *= text_scale;
    theme_value.radius *= text_scale;
    theme_value.radius_small *= text_scale;
    theme_value.radius_medium *= text_scale;
    theme_value.radius_large *= text_scale;
    theme_value.radius_panel *= text_scale;
    theme_value.radius_pill *= text_scale;
    return theme_value;
}

Theme Theme::with_radius_scale(float scale) const
{
    Theme theme_value = *this;
    const float k = scale < 0.0f ? 0.0f : scale;
    theme_value.radius *= k;
    theme_value.radius_small *= k;
    theme_value.radius_medium *= k;
    theme_value.radius_large *= k;
    theme_value.radius_panel *= k;
    // pill 是「足够大 + clamp 到短边一半」的语义值：保持大值，只在 0 时变成直角。
    if (k <= 0.0f) theme_value.radius_pill = 0.0f;
    return theme_value;
}

Theme Theme::with_transition_ms(float ms) const
{
    Theme theme_value = *this;
    theme_value.transition_ms = ms < 0.0f ? 0.0f : ms;
    return theme_value;
}

Theme Theme::with_focus_policy(FocusPolicy policy) const
{
    Theme theme_value = *this;
    theme_value.focus_policy = policy;
    return theme_value;
}

Theme Theme::with_rounding(bool rounded) const
{
    Theme theme_value = *this;
    if (rounded) {
        theme_value.focus_style = FocusStyle::Outline;
        return theme_value;
    }
    theme_value.radius = 0.0f;
    theme_value.radius_small = 0.0f;
    theme_value.radius_medium = 0.0f;
    theme_value.radius_large = 0.0f;
    theme_value.radius_panel = 0.0f;
    theme_value.radius_pill = 0.0f;
    // 内阴影属于圆角视觉的一部分：classic 完全回到旧观感。
    theme_value.inset_shadow_size = 0.0f;
    theme_value.focus_style = FocusStyle::Border;
    return theme_value;
}

Theme Theme::light()
{
    Theme t;
    t.name = "light";

    t.window_bg       = srgb_hex(0xEDEFF2);
    t.panel_bg        = srgb_hex(0xDEE1E5);
    t.panel_border    = srgb_hex(0xB3B8BF);
    t.elevated        = srgb_hex(0xFAFBFC);
    t.elevated_border = srgb_hex(0xA8AEB6);
    t.splitter        = srgb_hex(0xC3C7CD);
    t.text            = srgb_hex(0x1A1D21);
    t.text_disabled   = srgb_hex(0x6B7178);
    t.text_muted      = srgb_hex(0x5C6269);

    t.button_bg       = srgb_hex(0xC8CCD2);
    t.button_hover    = srgb_hex(0xDCE0E5);
    t.button_pressed  = srgb_hex(0xB4B9C0);
    t.button_disabled = srgb_hex(0xE0E3E7);
    t.focus_ring      = srgb_hex(0x1F6FD0);
    t.accent          = srgb_hex(0x1F6FD0);
    t.accent_text     = srgb_hex(0x1A5FB4);

    t.checkbox_bg     = srgb_hex(0xF7F8FA);
    t.checkbox_border = srgb_hex(0x9EA3AA);
    t.checkbox_fill   = srgb_hex(0x1F6FD0);

    t.slider_track    = srgb_hex(0xC7CBD1);
    t.slider_fill     = srgb_hex(0x1F6FD0);
    t.slider_handle   = srgb_hex(0x292D33);
    t.track_hover     = srgb_hex(0xADB2B9);

    t.text_input_bg           = srgb_hex(0xFAFBFC);
    t.text_input_border       = srgb_hex(0xADB2B9);
    t.text_input_selection    = srgb_hex(0x1F6FD0, 0.30f);
    t.text_input_cursor       = srgb_hex(0x0D0F11);
    t.text_input_border_hover = srgb_hex(0x727880);

    t.tooltip_bg      = srgb_hex(0xF7F8FA, 0.98f);
    t.scrollbar_track = srgb_hex(0xCCD0D5);
    t.scrollbar_thumb = srgb_hex(0x8C9299);
    t.inset_shadow    = {0.0f, 0.0f, 0.0f, 0.05f};
    return t;
}
} // namespace xel::ui
