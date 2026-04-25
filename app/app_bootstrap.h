#pragma once

#include "app/app_services.h"
#include "logging/logger.h"

namespace app::modules {
class IAppModule;
}

namespace app {

class AppBootstrap final {
   public:
    explicit AppBootstrap(const AppServices& services);

    [[nodiscard]] app::VoidResult<QString> MigrateSettings() const;
    [[nodiscard]] logging::LoggerConfig DefaultLoggerConfig(
        logging::LogSinkType sink_type = logging::LogSinkType::kNone) const;

    void InitializeLogging(const logging::LoggerConfig& config);
    void ShutdownLogging();
    void RegisterModule(modules::IAppModule& module) const;

   private:
    AppServices services_;
    bool logging_initialized_{false};
};

}  // namespace app
