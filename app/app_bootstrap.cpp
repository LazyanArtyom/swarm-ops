#include "app/app_bootstrap.h"

#include <QDir>
#include <QStandardPaths>

#include "app/modules/app_module.h"
#include "core/configs/app_configs.h"
#include "logging/qt_message_handler.h"

namespace app {

AppBootstrap::AppBootstrap(const AppServices& services) : services_(services) {}

app::VoidResult<QString> AppBootstrap::MigrateSettings() const {
    return services_.Settings().Migrate();
}

logging::LoggerConfig AppBootstrap::DefaultLoggerConfig(logging::LogSinkType sink_type) const {
    logging::LoggerConfig config;
    config.logger_name = services_.Info().slug.toStdString();
    config.default_category = services_.Info().slug.toStdString();
    config.sink_type = sink_type;

    const QString log_dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString log_file_path =
        log_dir.isEmpty()
            ? QStringLiteral("%1.log").arg(services_.Info().slug)
            : QDir(log_dir).filePath(QStringLiteral("%1.log").arg(services_.Info().slug));
    config.log_file_path = log_file_path.toStdString();
    return services_.Settings().Logging().LoadLoggerConfig(config);
}

void AppBootstrap::InitializeLogging(const logging::LoggerConfig& config) {
    logging::Logger::Init(config);
    logging::QtMessageHandlerBridge::Install();
    logging_initialized_ = true;
}

void AppBootstrap::ShutdownLogging() {
    if (!logging_initialized_) {
        return;
    }

    logging::QtMessageHandlerBridge::Uninstall();
    logging::Logger::Shutdown();
    logging_initialized_ = false;
}

void AppBootstrap::RegisterModule(modules::IAppModule& module) const {
    module.RegisterPages(services_.Pages());
    module.RegisterCommands(services_.Commands());
}

}  // namespace app
