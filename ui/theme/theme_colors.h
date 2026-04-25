#pragma once

#include <QColor>
#include <cstdint>

namespace app::ui::theme {

enum class ThemeId : std::uint8_t { kDark, kLight };

struct ThemeColors final {
    QColor fg;
    QColor fg_muted;
    QColor fg_disabled;
    QColor bg_window;
    QColor bg_surface;
    QColor bg_surface_alt;
    QColor bg_elevated;
    QColor border;
    QColor border_strong;
    QColor primary;
    QColor on_primary;
    QColor primary_container;
    QColor on_primary_container;
    QColor secondary;
    QColor on_secondary;
    QColor secondary_container;
    QColor on_secondary_container;
    QColor info;
    QColor on_info;
    QColor success;
    QColor on_success;
    QColor warning;
    QColor on_warning;
    QColor error;
    QColor on_error;
    QColor state_hover;
    QColor state_pressed;
    QColor state_selected;
    QColor tooltip_bg;
    QColor tooltip_fg;
    QColor console_metadata;

    static ThemeColors Dark();
    static ThemeColors Light();
    static ThemeColors For(ThemeId theme_id);
};

}  // namespace app::ui::theme
