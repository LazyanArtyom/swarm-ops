#include "theme/theme_colors.h"

#include <string_view>

namespace app::ui::theme {
namespace {

QColor Hex(std::string_view value) {
    return {QString::fromLatin1(value.data(), static_cast<qsizetype>(value.size()))};
}

struct BasePaletteTokens final {
    std::string_view fg;
    std::string_view fg_muted;
    std::string_view fg_disabled;
    std::string_view bg_window;
    std::string_view bg_surface;
    std::string_view bg_surface_alt;
    std::string_view bg_elevated;
    std::string_view border;
    std::string_view border_strong;
};

struct IntentPaletteTokens final {
    std::string_view primary;
    std::string_view on_primary;
    std::string_view primary_container;
    std::string_view on_primary_container;
    std::string_view secondary;
    std::string_view on_secondary;
    std::string_view secondary_container;
    std::string_view on_secondary_container;
    std::string_view info;
    std::string_view on_info;
    std::string_view success;
    std::string_view on_success;
    std::string_view warning;
    std::string_view on_warning;
    std::string_view error;
    std::string_view on_error;
};

struct InteractionPaletteTokens final {
    std::string_view state_hover;
    std::string_view state_pressed;
    std::string_view state_selected;
    std::string_view tooltip_bg;
    std::string_view tooltip_fg;
    std::string_view console_metadata;
};

struct ThemePaletteTokens final {
    BasePaletteTokens base;
    IntentPaletteTokens intent;
    InteractionPaletteTokens interaction;
};

constexpr ThemePaletteTokens kDarkTokens{
    .base =
        {
            .fg = "#FFCCCCCC",
            .fg_muted = "#FF8C8C8C",
            .fg_disabled = "#FF666666",
            .bg_window = "#FF1F2125",
            .bg_surface = "#FF24262A",
            .bg_surface_alt = "#FF24262A",
            .bg_elevated = "#FF24262A",
            .border = "#FF2B2B2B",
            .border_strong = "#FF424242",
        },
    .intent =
        {
            .primary = "#FF4285F4",
            .on_primary = "#FFE8F0FE",
            .primary_container = "#FF4285F4",
            .on_primary_container = "#FFE8F0FE",
            .secondary = "#FF4EC9B0",
            .on_secondary = "#FF001F18",
            .secondary_container = "#FF173E37",
            .on_secondary_container = "#FFD8FFF7",
            .info = "#FF3794FF",
            .on_info = "#FF001F33",
            .success = "#FF89D185",
            .on_success = "#FF0F2A0E",
            .warning = "#FFFFCC66",
            .on_warning = "#FF332200",
            .error = "#FFF48771",
            .on_error = "#FF3A0900",
        },
    .interaction =
        {
            .state_hover = "#294285F4",
            .state_pressed = "#474285F4",
            .state_selected = "#474285F4",
            .tooltip_bg = "#FF202020",
            .tooltip_fg = "#FFCCCCCC",
            .console_metadata = "#FF90A4AE",
        },
};

constexpr ThemePaletteTokens kLightTokens{
    .base =
        {
            .fg = "#FF202124",
            .fg_muted = "#FF5F6368",
            .fg_disabled = "#FF9AA0A6",
            .bg_window = "#FFF3F4F6",
            .bg_surface = "#FFFFFFFF",
            .bg_surface_alt = "#FFF8F9FA",
            .bg_elevated = "#FFFFFFFF",
            .border = "#FFE0E3E7",
            .border_strong = "#FFC7CDD4",
        },
    .intent =
        {
            .primary = "#FF1976D2",
            .on_primary = "#FFFFFFFF",
            .primary_container = "#FF1976D2",
            .on_primary_container = "#FFFFFFFF",
            .secondary = "#FF00897B",
            .on_secondary = "#FFFFFFFF",
            .secondary_container = "#FFE0F2F1",
            .on_secondary_container = "#FF00695C",
            .info = "#FF1A73E8",
            .on_info = "#FFFFFFFF",
            .success = "#FF188038",
            .on_success = "#FFFFFFFF",
            .warning = "#FFF9AB00",
            .on_warning = "#FF202124",
            .error = "#FFD93025",
            .on_error = "#FFFFFFFF",
        },
    .interaction =
        {
            .state_hover = "#291976D2",
            .state_pressed = "#471976D2",
            .state_selected = "#471976D2",
            .tooltip_bg = "#FFFFFFFF",
            .tooltip_fg = "#FF202124",
            .console_metadata = "#FF607D8B",
        },
};

ThemeColors FromTokens(const ThemePaletteTokens& tokens) {
    return {
        .fg = Hex(tokens.base.fg),
        .fg_muted = Hex(tokens.base.fg_muted),
        .fg_disabled = Hex(tokens.base.fg_disabled),
        .bg_window = Hex(tokens.base.bg_window),
        .bg_surface = Hex(tokens.base.bg_surface),
        .bg_surface_alt = Hex(tokens.base.bg_surface_alt),
        .bg_elevated = Hex(tokens.base.bg_elevated),
        .border = Hex(tokens.base.border),
        .border_strong = Hex(tokens.base.border_strong),
        .primary = Hex(tokens.intent.primary),
        .on_primary = Hex(tokens.intent.on_primary),
        .primary_container = Hex(tokens.intent.primary_container),
        .on_primary_container = Hex(tokens.intent.on_primary_container),
        .secondary = Hex(tokens.intent.secondary),
        .on_secondary = Hex(tokens.intent.on_secondary),
        .secondary_container = Hex(tokens.intent.secondary_container),
        .on_secondary_container = Hex(tokens.intent.on_secondary_container),
        .info = Hex(tokens.intent.info),
        .on_info = Hex(tokens.intent.on_info),
        .success = Hex(tokens.intent.success),
        .on_success = Hex(tokens.intent.on_success),
        .warning = Hex(tokens.intent.warning),
        .on_warning = Hex(tokens.intent.on_warning),
        .error = Hex(tokens.intent.error),
        .on_error = Hex(tokens.intent.on_error),
        .state_hover = Hex(tokens.interaction.state_hover),
        .state_pressed = Hex(tokens.interaction.state_pressed),
        .state_selected = Hex(tokens.interaction.state_selected),
        .tooltip_bg = Hex(tokens.interaction.tooltip_bg),
        .tooltip_fg = Hex(tokens.interaction.tooltip_fg),
        .console_metadata = Hex(tokens.interaction.console_metadata),
    };
}

}  // namespace

ThemeColors ThemeColors::Dark() {
    return FromTokens(kDarkTokens);
}

ThemeColors ThemeColors::Light() {
    return FromTokens(kLightTokens);
}

ThemeColors ThemeColors::For(ThemeId theme_id) {
    return (theme_id == ThemeId::kDark) ? Dark() : Light();
}

}  // namespace app::ui::theme
