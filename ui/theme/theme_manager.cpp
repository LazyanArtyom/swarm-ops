#include "theme/theme_manager.h"

#include <QApplication>

#include "theme/fonts_config.h"
#include "theme/theme_icons.h"
#include "theme/theme_metrics.h"

namespace app::ui::theme {
namespace {}  // namespace

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;

    if (!instance.wired_) {
        instance.wired_ = true;

        connect(&ThemeMetrics::Instance(), &ThemeMetrics::SigMetricsChanged, &instance,
                &ThemeManager::ApplyToApp);
    }

    return instance;
}

ThemeId ThemeManager::CurrentThemeId() const {
    return theme_id_;
}

ThemeColors ThemeManager::CurrentColors() const {
    return ThemeColors::For(theme_id_);
}

void ThemeManager::Apply(ThemeId theme_id) {
    theme_id_ = theme_id;

    ApplyToApp();

    emit SigThemeChanged(theme_id_);
}

void ThemeManager::Toggle() {
    Apply(theme_id_ == ThemeId::kDark ? ThemeId::kLight : ThemeId::kDark);
}

void ThemeManager::ApplyToApp() {
    ThemeMetrics::Instance().Refresh();
    fonts::Init();

    const ThemeColors colors = CurrentColors();
    const ThemeLoader::TokenMap tokens =
        ThemeLoader::BuildTokens(colors, ThemeMetrics::Instance().Current());

    qApp->setProperty("themeId", (theme_id_ == ThemeId::kDark) ? QStringLiteral("dark")
                                                               : QStringLiteral("light"));
    qApp->setPalette(MakePalette(colors));
    ThemeLoader::ApplyToApp(QssPaths(), tokens);
    ThemeIcons::Instance().Refresh(theme_id_);
}

QStringList ThemeManager::QssPaths() {
    return {QStringLiteral(":/theme/qss/common.qss")};
}

QPalette ThemeManager::MakePalette(const ThemeColors& colors) {
    QPalette palette;
    palette.setColor(QPalette::Window, colors.bg_window);
    palette.setColor(QPalette::WindowText, colors.fg);
    palette.setColor(QPalette::Base, colors.bg_surface);
    palette.setColor(QPalette::AlternateBase, colors.bg_surface_alt);
    palette.setColor(QPalette::Text, colors.fg);
    palette.setColor(QPalette::Button, colors.bg_surface);
    palette.setColor(QPalette::ButtonText, colors.fg);
    palette.setColor(QPalette::PlaceholderText, colors.fg_muted);
    palette.setColor(QPalette::Highlight, colors.primary);
    palette.setColor(QPalette::HighlightedText, colors.on_primary);
    palette.setColor(QPalette::Mid, colors.border);
    palette.setColor(QPalette::Midlight, colors.border_strong);
    palette.setColor(QPalette::ToolTipBase, colors.tooltip_bg);
    palette.setColor(QPalette::ToolTipText, colors.tooltip_fg);
    palette.setColor(QPalette::BrightText, colors.error);
    palette.setColor(QPalette::Link, colors.primary);
    palette.setColor(QPalette::LinkVisited, colors.secondary);
    return palette;
}

}  // namespace app::ui::theme
