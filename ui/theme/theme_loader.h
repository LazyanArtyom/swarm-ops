#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

#include "theme/theme_colors.h"
#include "theme/theme_metrics.h"
#include "utils/result.h"

namespace app::ui::theme {

class ThemeLoader final {
   public:
    using TokenMap = QHash<QString, QString>;
    using QssResult = app::Result<QString, QString>;

    static QssResult TryReadQss(const QString& path);
    static QString ReadQss(const QString& path);

    static TokenMap BuildTokens(const ThemeColors& colors, const ThemeMetricsData& metrics);
    static QString Sub(QString str, const TokenMap& tokens);
    static QssResult TryLoadQss(const QStringList& paths, const TokenMap& tokens);
    static QString LoadQss(const QStringList& paths, const TokenMap& tokens);

    static QssResult TryApplyToApp(const QStringList& paths, const TokenMap& tokens);
    static void ApplyToApp(const QStringList& paths, const TokenMap& tokens);
};

}  // namespace app::ui::theme
