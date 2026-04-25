#include "theme/theme_loader.h"

#include <QApplication>
#include <QFile>
#include <QHashIterator>
#include <QRegularExpression>
#include <QSet>
#include <QStringConverter>
#include <QTextStream>
#include <expected>

#include "logging/logger.h"
#include "theme/fonts_config.h"

namespace app::ui::theme {
namespace {

constexpr std::string_view kThemeLogCategory = "theme";
constexpr auto kUnresolvedTokenPattern = R"(\$\{[A-Z0-9_]+\})";

std::string ToStdString(const QString& value) {
    return value.toStdString();
}

void ReplaceToken(QString* str, const QString& key, const QString& value) {
    if (str != nullptr) {
        str->replace(key, value);
    }
}

QString Px(int value) {
    return QString::number(value);
}

QStringList FindUnresolvedTokens(const QString& qss) {
    const QRegularExpression token_expression(QString::fromLatin1(kUnresolvedTokenPattern));
    QRegularExpressionMatchIterator token_matches = token_expression.globalMatch(qss);

    QSet<QString> unique_tokens;
    while (token_matches.hasNext()) {
        unique_tokens.insert(token_matches.next().captured());
    }

    QStringList tokens{unique_tokens.begin(), unique_tokens.end()};
    tokens.sort(Qt::CaseInsensitive);
    return tokens;
}

}  // namespace

ThemeLoader::QssResult ThemeLoader::TryReadQss(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::unexpected(
            QStringLiteral("Failed to open QSS file '%1': %2").arg(path, file.errorString()));
    }
    QTextStream text_stream(&file);
    text_stream.setEncoding(QStringConverter::Utf8);
    return text_stream.readAll();
}

QString ThemeLoader::ReadQss(const QString& path) {
    const QssResult result = TryReadQss(path);
    return result ? *result : QString();
}

ThemeLoader::TokenMap ThemeLoader::BuildTokens(const ThemeColors& colors,
                                               const ThemeMetricsData& metrics) {
    const QString ui_family = fonts::UiFamily();
    const QString mono_family = fonts::MonoFamily();

    return {
        {QStringLiteral("${FG}"), colors.fg.name()},
        {QStringLiteral("${FG_MUTED}"), colors.fg_muted.name(QColor::HexArgb)},
        {QStringLiteral("${FG_DISABLED}"), colors.fg_disabled.name(QColor::HexArgb)},
        {QStringLiteral("${BG_WINDOW}"), colors.bg_window.name()},
        {QStringLiteral("${BG_SURFACE}"), colors.bg_surface.name()},
        {QStringLiteral("${BG_SURFACE_ALT}"), colors.bg_surface_alt.name()},
        {QStringLiteral("${BG_ELEVATED}"), colors.bg_elevated.name()},
        {QStringLiteral("${BORDER}"), colors.border.name()},
        {QStringLiteral("${BORDER_STRONG}"), colors.border_strong.name()},
        {QStringLiteral("${PRIMARY}"), colors.primary.name()},
        {QStringLiteral("${ON_PRIMARY}"), colors.on_primary.name()},
        {QStringLiteral("${PRIMARY_CONTAINER}"), colors.primary_container.name()},
        {QStringLiteral("${ON_PRIMARY_CONTAINER}"), colors.on_primary_container.name()},
        {QStringLiteral("${SECONDARY}"), colors.secondary.name()},
        {QStringLiteral("${ON_SECONDARY}"), colors.on_secondary.name()},
        {QStringLiteral("${SECONDARY_CONTAINER}"), colors.secondary_container.name()},
        {QStringLiteral("${ON_SECONDARY_CONTAINER}"), colors.on_secondary_container.name()},
        {QStringLiteral("${INFO}"), colors.info.name()},
        {QStringLiteral("${ON_INFO}"), colors.on_info.name()},
        {QStringLiteral("${SUCCESS}"), colors.success.name()},
        {QStringLiteral("${ON_SUCCESS}"), colors.on_success.name()},
        {QStringLiteral("${WARNING}"), colors.warning.name()},
        {QStringLiteral("${ON_WARNING}"), colors.on_warning.name()},
        {QStringLiteral("${ERROR}"), colors.error.name()},
        {QStringLiteral("${ON_ERROR}"), colors.on_error.name()},
        {QStringLiteral("${STATE_HOVER}"), colors.state_hover.name(QColor::HexArgb)},
        {QStringLiteral("${STATE_PRESSED}"), colors.state_pressed.name(QColor::HexArgb)},
        {QStringLiteral("${STATE_SELECTED}"), colors.state_selected.name(QColor::HexArgb)},
        {QStringLiteral("${TOOLTIP_BG}"), colors.tooltip_bg.name()},
        {QStringLiteral("${TOOLTIP_FG}"), colors.tooltip_fg.name()},
        {QStringLiteral("${RADIUS_SM}"), Px(metrics.radius_sm_px)},
        {QStringLiteral("${RADIUS_MD}"), Px(metrics.radius_md_px)},
        {QStringLiteral("${SPACE_XS}"), Px(metrics.spacing_xs_px)},
        {QStringLiteral("${SPACE_SM}"), Px(metrics.spacing_sm_px)},
        {QStringLiteral("${SPACE_MD}"), Px(metrics.spacing_md_px)},
        {QStringLiteral("${SPACE_LG}"), Px(metrics.spacing_lg_px)},
        {QStringLiteral("${ICON_SM_PX}"), Px(metrics.icon_sm_px)},
        {QStringLiteral("${ICON_MD_PX}"), Px(metrics.icon_md_px)},
        {QStringLiteral("${CONTROL_HEIGHT_PX}"), Px(metrics.control_height_px)},
        {QStringLiteral("${DIALOG_BUTTON_MIN_HEIGHT_PX}"), Px(metrics.dialog_button_min_height_px)},
        {QStringLiteral("${DIALOG_BUTTON_MIN_WIDTH_PX}"), Px(metrics.dialog_button_min_width_px)},
        {QStringLiteral("${DIALOG_BUTTON_PADDING_X_PX}"), Px(metrics.dialog_button_padding_x_px)},
        {QStringLiteral("${DIALOG_BUTTON_PADDING_Y_PX}"), Px(metrics.dialog_button_padding_y_px)},
        {QStringLiteral("${SETTINGS_FIELD_HEIGHT_PX}"), Px(metrics.settings_field_height_px)},
        {QStringLiteral("${SETTINGS_POPUP_MIN_HEIGHT_PX}"),
         Px(metrics.settings_popup_min_height_px)},
        {QStringLiteral("${CONSOLE_HEADER_CONTROL_HEIGHT_PX}"),
         Px(metrics.console_header_control_height_px)},
        {QStringLiteral("${SCROLLBAR_EXTENT_PX}"), Px(metrics.scrollbar_extent_px)},
        {QStringLiteral("${SCROLLBAR_MIN_SLIDER_PX}"), Px(metrics.scrollbar_min_slider_px)},
        {QStringLiteral("${FONT_BASE_PT}"), Px(metrics.font_base_pt)},
        {QStringLiteral("${FONT_MONO_PT}"), Px(metrics.font_mono_pt)},
        {QStringLiteral("${FONT_TOOLTIP_PT}"), Px(metrics.font_tooltip_pt)},
        {QStringLiteral("${FONT_UI_FAMILY}"), ui_family},
        {QStringLiteral("${FONT_MONO_FAMILY}"), mono_family},
        {QStringLiteral("${TOOLTIP_PADDING_X_PX}"), Px(metrics.tooltip_padding_x_px)},
        {QStringLiteral("${TOOLTIP_PADDING_Y_PX}"), Px(metrics.tooltip_padding_y_px)},
        {QStringLiteral("${TOOLBAR_BUTTON_PADDING_X_PX}"), Px(metrics.toolbar_button_padding_x_px)},
        {QStringLiteral("${TOOLBAR_BUTTON_PADDING_Y_PX}"), Px(metrics.toolbar_button_padding_y_px)},
    };
}

QString ThemeLoader::Sub(QString str, const TokenMap& tokens) {
    QHashIterator<QString, QString> token_it(tokens);
    while (token_it.hasNext()) {
        token_it.next();
        ReplaceToken(&str, token_it.key(), token_it.value());
    }
    return str;
}

ThemeLoader::QssResult ThemeLoader::TryLoadQss(const QStringList& paths, const TokenMap& tokens) {
    QString all_qss;
    for (const auto& path : paths) {
        const QssResult chunk_result = TryReadQss(path);
        if (!chunk_result) {
            return std::unexpected(chunk_result.error());
        }

        const QString& chunk = *chunk_result;
        if (!chunk.isEmpty()) {
            all_qss += chunk;
            all_qss += QLatin1Char('\n');
        }
    }

    QString rendered_qss = Sub(all_qss, tokens);
    const QStringList unresolved_tokens = FindUnresolvedTokens(rendered_qss);
    if (!unresolved_tokens.isEmpty()) {
        return std::unexpected(QStringLiteral("Unresolved theme token(s): %1")
                                   .arg(unresolved_tokens.join(QStringLiteral(", "))));
    }

    return rendered_qss;
}

QString ThemeLoader::LoadQss(const QStringList& paths, const TokenMap& tokens) {
    const QssResult result = TryLoadQss(paths, tokens);
    return result ? *result : QString();
}

ThemeLoader::QssResult ThemeLoader::TryApplyToApp(const QStringList& paths,
                                                  const TokenMap& tokens) {
    if (qApp == nullptr) {
        return std::unexpected(QStringLiteral("QApplication is not initialized"));
    }

    const QssResult result = TryLoadQss(paths, tokens);
    if (!result) {
        return std::unexpected(result.error());
    }

    qApp->setStyleSheet(*result);
    return result;
}

void ThemeLoader::ApplyToApp(const QStringList& paths, const TokenMap& tokens) {
    const QssResult result = TryApplyToApp(paths, tokens);
    if (!result) {
        app::logging::Logger::ErrorFmtFor(kThemeLogCategory, "Failed to apply theme QSS: {}",
                                          ToStdString(result.error()));
        if (qApp != nullptr) {
            qApp->setStyleSheet(QString());
        }
        return;
    }

    app::logging::Logger::DebugFmtFor(kThemeLogCategory, "Applied theme QSS from {} file(s)",
                                      paths.size());
}

}  // namespace app::ui::theme
