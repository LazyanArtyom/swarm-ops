#include "app/app_context.h"

#include <QDebug>
#include <utility>

namespace app {

AppContext::AppContext(AppInfo info, QObject* parent)
    : AppContext(std::move(info), {}, parent) {}

AppContext::AppContext(AppInfo info, AppContextDependencies dependencies, QObject* parent)
    : QObject(parent),
      info_(std::move(info)),
      settings_(dependencies.settings != nullptr ? dependencies.settings
                                                 : &configs::AppSettings::Default()),
      theme_manager_(dependencies.theme_manager != nullptr ? dependencies.theme_manager
                                                           : &ui::theme::ThemeManager::Instance()),
      page_registry_(dependencies.page_registry != nullptr ? dependencies.page_registry
                                                           : &page_registry_storage_),
      command_registry_(dependencies.command_registry != nullptr ? dependencies.command_registry
                                                                 : &command_registry_storage_),
      services_(info_, *settings_, *theme_manager_, *page_registry_, *command_registry_),
      bootstrap_(services_) {
    const app::VoidResult<QString> migration_result = bootstrap_.MigrateSettings();
    if (!migration_result) {
        qWarning().noquote() << migration_result.error();
    }
}

const AppServices& AppContext::Services() const {
    return services_;
}

AppBootstrap& AppContext::Bootstrap() {
    return bootstrap_;
}

const AppBootstrap& AppContext::Bootstrap() const {
    return bootstrap_;
}

}  // namespace app
