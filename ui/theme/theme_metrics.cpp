#include "theme/theme_metrics.h"

#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <algorithm>

namespace app::ui::theme {
namespace {

#if defined(Q_OS_MACOS)
constexpr qreal kBaselineLogicalDpi = 72.0;
#else
constexpr qreal kBaselineLogicalDpi = 96.0;
#endif

constexpr qreal kFallbackLogicalDpi = kBaselineLogicalDpi;

struct MetricSpec final {
    int base = 0;
    int min = 0;
    int max = 0;
};

// Raw design tokens: primitive scale-aware values used across components.
struct TypographySpecs final {
#if defined(Q_OS_MACOS)
    MetricSpec base_font_pt{.base = 10, .min = 11, .max = 18};
#else
    MetricSpec base_font_pt{.base = 10, .min = 9, .max = 16};
#endif
};

struct ShapeSpecs final {
    MetricSpec radius_sm_px{.base = 6, .min = 4, .max = 12};
    MetricSpec radius_md_px{.base = 8, .min = 4, .max = 12};
};

struct SpacingSpecs final {
    MetricSpec xs_px{.base = 4, .min = 4, .max = 24};
    MetricSpec sm_px{.base = 8, .min = 4, .max = 24};
    MetricSpec md_px{.base = 12, .min = 4, .max = 24};
    MetricSpec lg_px{.base = 16, .min = 4, .max = 24};
};

struct IconSpecs final {
    MetricSpec sm_px{.base = 16, .min = 14, .max = 36};
    MetricSpec md_px{.base = 20, .min = 14, .max = 36};
};

struct ControlSpecs final {
    MetricSpec height_px{.base = 32, .min = 28, .max = 48};
};

// Semantic component tokens: resolved into QSS/API metrics for specific widgets.
struct WorkspaceSpecs final {
    MetricSpec side_panel_width_px{.base = 280, .min = 120, .max = 520};
    MetricSpec side_panel_min_width_px{.base = 140, .min = 120, .max = 520};
    MetricSpec central_width_px{.base = 720, .min = 480, .max = 1600};
    MetricSpec central_min_width_px{.base = 360, .min = 320, .max = 960};
    MetricSpec workspace_height_px{.base = 720, .min = 320, .max = 1600};
    MetricSpec bottom_panel_height_px{.base = 180, .min = 80, .max = 360};
    MetricSpec bottom_panel_min_height_px{.base = 90, .min = 80, .max = 360};
};

struct MainWindowSpecs final {
    MetricSpec fallback_min_width_px{.base = 640, .min = 640, .max = 1280};
    MetricSpec fallback_min_height_px{.base = 420, .min = 420, .max = 900};
    double max_screen_width_ratio{0.9};
    double max_screen_height_ratio{0.85};
};

struct DialogSpecs final {
    MetricSpec message_min_width_px{.base = 352, .min = 280, .max = 560};
    MetricSpec details_min_height_px{.base = 128, .min = 96, .max = 240};
    MetricSpec button_min_height_px{.base = 22, .min = 22, .max = 28};
    MetricSpec button_min_width_px{.base = 56, .min = 52, .max = 80};
    MetricSpec button_padding_x_px{.base = 8, .min = 6, .max = 12};
    MetricSpec button_padding_y_px{.base = 2, .min = 2, .max = 5};
};

struct SettingsSpecs final {
    MetricSpec dialog_min_width_px{.base = 620, .min = 560, .max = 960};
    MetricSpec dialog_min_height_px{.base = 380, .min = 360, .max = 720};
    MetricSpec navigation_width_px{.base = 168, .min = 144, .max = 240};
    MetricSpec label_width_px{.base = 96, .min = 80, .max = 144};
    MetricSpec field_min_width_px{.base = 220, .min = 180, .max = 360};
    MetricSpec field_height_px{.base = 28, .min = 28, .max = 36};
    MetricSpec popup_min_height_px{.base = 64, .min = 64, .max = 180};
};

struct ConsoleChromeSpecs final {
    MetricSpec control_height_px{.base = 22, .min = 22, .max = 30};
    MetricSpec level_filter_min_width_px{.base = 88, .min = 72, .max = 160};
    MetricSpec search_min_width_px{.base = 140, .min = 112, .max = 280};
    MetricSpec search_max_width_px{.base = 220, .min = 160, .max = 420};
    MetricSpec close_button_extent_px{.base = 22, .min = 20, .max = 32};
};

struct ScrollbarSpecs final {
    MetricSpec extent_px{.base = 10, .min = 8, .max = 16};
    MetricSpec min_slider_px{.base = 24, .min = 18, .max = 40};
};

struct TooltipSpecs final {
    MetricSpec font_pt{.base = 9, .min = 9, .max = 12};
    MetricSpec padding_x_px{.base = 8, .min = 6, .max = 14};
    MetricSpec padding_y_px{.base = 5, .min = 4, .max = 10};
};

struct ToolbarSpecs final {
    MetricSpec button_padding_x_px{.base = 10, .min = 8, .max = 18};
    MetricSpec button_padding_y_px{.base = 6, .min = 5, .max = 12};
};

struct DesignTokenSpecs final {
    TypographySpecs typography;
    ShapeSpecs shape;
    SpacingSpecs spacing;
    IconSpecs icon;
    ControlSpecs control;
};

struct ComponentMetricSpecs final {
    MainWindowSpecs main_window;
    WorkspaceSpecs workspace;
    DialogSpecs dialog;
    SettingsSpecs settings;
    ConsoleChromeSpecs console_chrome;
    ScrollbarSpecs scrollbar;
    TooltipSpecs tooltip;
    ToolbarSpecs toolbar;
};

struct ThemeMetricSpecs final {
    DesignTokenSpecs design;
    ComponentMetricSpecs component;
};

constexpr ThemeMetricSpecs kMetricSpecs;

[[nodiscard]] int ClampInt(int val, int low, int high) {
    if (val < low) {
        return low;
    }
    if (val > high) {
        return high;
    }
    return val;
}

[[nodiscard]] int ScaleAndRoundInt(int base, qreal scale) {
    constexpr qreal round_bias = 0.5;
    return static_cast<int>((static_cast<qreal>(base) * scale) + round_bias);
}

[[nodiscard]] int Resolve(MetricSpec spec, qreal scale) {
    return ClampInt(ScaleAndRoundInt(spec.base, scale), spec.min, spec.max);
}

[[nodiscard]] const QScreen* BestScreen() {
    if (QWindow* window = QGuiApplication::focusWindow();
        window != nullptr && window->screen() != nullptr) {
        return window->screen();
    }

    const auto windows = QGuiApplication::topLevelWindows();
    if (!windows.isEmpty() && windows.front() != nullptr && windows.front()->screen() != nullptr) {
        return windows.front()->screen();
    }

    return QGuiApplication::primaryScreen();
}

[[nodiscard]] qreal EffectiveLogicalDpi() {
    const QScreen* screen = BestScreen();
    const qreal dpi = (screen != nullptr) ? screen->logicalDotsPerInch() : kFallbackLogicalDpi;

    constexpr qreal min_dpi = 1.0;
    return (dpi > min_dpi) ? dpi : kFallbackLogicalDpi;
}

}  // namespace

ThemeMetrics& ThemeMetrics::Instance() {
    static ThemeMetrics instance;
    return instance;
}

ThemeMetricsData ThemeMetrics::Current() const {
    return metrics_;
}

void ThemeMetrics::Refresh() {
    const qreal dpi = EffectiveLogicalDpi();
    const qreal scale = dpi / kBaselineLogicalDpi;

    ThemeMetricsData next;

    next.font_base_pt = Resolve(kMetricSpecs.design.typography.base_font_pt, scale);
    next.font_mono_pt = next.font_base_pt;
    next.font_tooltip_pt = Resolve(kMetricSpecs.component.tooltip.font_pt, scale);

    next.radius_sm_px = Resolve(kMetricSpecs.design.shape.radius_sm_px, scale);
    next.radius_md_px = Resolve(kMetricSpecs.design.shape.radius_md_px, scale);

    next.spacing_xs_px = Resolve(kMetricSpecs.design.spacing.xs_px, scale);
    next.spacing_sm_px = Resolve(kMetricSpecs.design.spacing.sm_px, scale);
    next.spacing_md_px = Resolve(kMetricSpecs.design.spacing.md_px, scale);
    next.spacing_lg_px = Resolve(kMetricSpecs.design.spacing.lg_px, scale);

    next.icon_sm_px = Resolve(kMetricSpecs.design.icon.sm_px, scale);
    next.icon_md_px = Resolve(kMetricSpecs.design.icon.md_px, scale);
    next.control_height_px = Resolve(kMetricSpecs.design.control.height_px, scale);

    next.navigation_width_px = Resolve(kMetricSpecs.component.workspace.side_panel_width_px, scale);
    next.navigation_min_width_px =
        Resolve(kMetricSpecs.component.workspace.side_panel_min_width_px, scale);
    next.central_width_px = Resolve(kMetricSpecs.component.workspace.central_width_px, scale);
    next.central_min_width_px =
        Resolve(kMetricSpecs.component.workspace.central_min_width_px, scale);
    next.info_width_px = Resolve(kMetricSpecs.component.workspace.side_panel_width_px, scale);
    next.info_min_width_px =
        Resolve(kMetricSpecs.component.workspace.side_panel_min_width_px, scale);
    next.workspace_height_px = Resolve(kMetricSpecs.component.workspace.workspace_height_px, scale);
    next.console_height_px =
        Resolve(kMetricSpecs.component.workspace.bottom_panel_height_px, scale);
    next.console_min_height_px =
        Resolve(kMetricSpecs.component.workspace.bottom_panel_min_height_px, scale);
    next.main_window_default_width_px =
        next.navigation_width_px + next.central_width_px + next.info_width_px;
    next.main_window_default_height_px = next.workspace_height_px + next.console_height_px;
    next.main_window_min_width_px =
        std::max(next.navigation_min_width_px + next.central_min_width_px + next.info_min_width_px,
                 Resolve(kMetricSpecs.component.main_window.fallback_min_width_px, scale));
    next.main_window_min_height_px =
        std::max(next.console_min_height_px + next.control_height_px + (next.spacing_lg_px * 2),
                 Resolve(kMetricSpecs.component.main_window.fallback_min_height_px, scale));
    next.main_window_max_screen_width_ratio =
        kMetricSpecs.component.main_window.max_screen_width_ratio;
    next.main_window_max_screen_height_ratio =
        kMetricSpecs.component.main_window.max_screen_height_ratio;

    next.dialog_icon_px =
        ClampInt(next.icon_md_px + next.spacing_sm_px, kMetricSpecs.design.icon.sm_px.min,
                 kMetricSpecs.design.icon.md_px.max + kMetricSpecs.design.spacing.lg_px.max);
    next.dialog_message_min_width_px =
        Resolve(kMetricSpecs.component.dialog.message_min_width_px, scale);
    next.dialog_details_min_height_px =
        Resolve(kMetricSpecs.component.dialog.details_min_height_px, scale);
    next.dialog_button_min_height_px =
        Resolve(kMetricSpecs.component.dialog.button_min_height_px, scale);
    next.dialog_button_min_width_px =
        Resolve(kMetricSpecs.component.dialog.button_min_width_px, scale);
    next.dialog_button_padding_x_px =
        Resolve(kMetricSpecs.component.dialog.button_padding_x_px, scale);
    next.dialog_button_padding_y_px =
        Resolve(kMetricSpecs.component.dialog.button_padding_y_px, scale);
    next.settings_dialog_min_width_px =
        Resolve(kMetricSpecs.component.settings.dialog_min_width_px, scale);
    next.settings_dialog_min_height_px =
        Resolve(kMetricSpecs.component.settings.dialog_min_height_px, scale);
    next.settings_navigation_width_px =
        Resolve(kMetricSpecs.component.settings.navigation_width_px, scale);
    next.settings_label_width_px = Resolve(kMetricSpecs.component.settings.label_width_px, scale);
    next.settings_field_min_width_px =
        Resolve(kMetricSpecs.component.settings.field_min_width_px, scale);
    next.settings_field_height_px =
        Resolve(kMetricSpecs.component.settings.field_height_px, scale);
    next.settings_popup_min_height_px =
        Resolve(kMetricSpecs.component.settings.popup_min_height_px, scale);
    next.console_header_control_height_px =
        Resolve(kMetricSpecs.component.console_chrome.control_height_px, scale);
    next.console_level_filter_min_width_px =
        Resolve(kMetricSpecs.component.console_chrome.level_filter_min_width_px, scale);
    next.console_search_min_width_px =
        Resolve(kMetricSpecs.component.console_chrome.search_min_width_px, scale);
    next.console_search_max_width_px =
        Resolve(kMetricSpecs.component.console_chrome.search_max_width_px, scale);
    next.panel_close_button_extent_px =
        Resolve(kMetricSpecs.component.console_chrome.close_button_extent_px, scale);

    next.scrollbar_extent_px = Resolve(kMetricSpecs.component.scrollbar.extent_px, scale);
    next.scrollbar_min_slider_px = Resolve(kMetricSpecs.component.scrollbar.min_slider_px, scale);

    next.tooltip_padding_x_px = Resolve(kMetricSpecs.component.tooltip.padding_x_px, scale);
    next.tooltip_padding_y_px = Resolve(kMetricSpecs.component.tooltip.padding_y_px, scale);

    next.toolbar_button_padding_x_px =
        Resolve(kMetricSpecs.component.toolbar.button_padding_x_px, scale);
    next.toolbar_button_padding_y_px =
        Resolve(kMetricSpecs.component.toolbar.button_padding_y_px, scale);

    const bool changed = (next != metrics_);

    metrics_ = next;

    if (changed) {
        emit SigMetricsChanged();
    }
}

}  // namespace app::ui::theme
