#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QString>
#include <string>

#include "app/app_context.h"
#include "app/app_info.h"
#include "app/app_translator.h"
#include "app/main_window.h"
#include "app/modules/home_module.h"
#include "logging/logger.h"
#include "ui/theme/theme_metrics.h"

namespace {

constexpr auto kUiLogCategory = "ui";

[[nodiscard]] std::string ToUtf8String(const QString& text) {
    return text.toUtf8().toStdString();
}

}  // namespace

int main(int argc, char** argv) {
    const app::AppInfo app_info = app::BuildAppInfo();

#if defined(Q_OS_LINUX)
    // qputenv("XDG_CURRENT_DESKTOP", "Unity");
    QGuiApplication::setDesktopFileName(app_info.slug);
#endif

    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    QApplication::setOrganizationName(app_info.vendor);
    QApplication::setApplicationName(app_info.display_name);
    QApplication::setApplicationVersion(app_info.version);
    QApplication::setStyle(QStringLiteral("fusion"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/theme/icons/app_icon.png")));

    app::AppContext context(app_info);
    context.Bootstrap().InitializeLogging(context.Bootstrap().DefaultLoggerConfig());

    app::AppTranslator translator;
    (void)translator.ApplyLanguage(context.Services().Settings().General().LanguageCode(),
                                   context.Services().Info());

    app::modules::HomeModule home_module;
    context.Bootstrap().RegisterModule(home_module);

    app::logging::Logger::Info(
        ToUtf8String(QCoreApplication::translate("AppLog", "Application started")));

    app::MainWindow window(context);
    window.setWindowTitle(context.Services().Info().WindowTitle());
    window.show();
    app::logging::Logger::InfoFor(
        kUiLogCategory,
        ToUtf8String(QCoreApplication::translate("AppLog", "Welcome to %1")
                         .arg(context.Services().Info().display_name)));

    // Ensure metrics are computed against the actual shown window/screen.
    app::ui::theme::ThemeMetrics::Instance().Refresh();

    const int exit_code = QApplication::exec();
    app::logging::Logger::Info(
        ToUtf8String(QCoreApplication::translate("AppLog", "Application exited")));
    context.Bootstrap().ShutdownLogging();
    return exit_code;
}
