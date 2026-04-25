#include "app/app_services.h"

#include "app/commands/command_registry.h"
#include "core/configs/app_configs.h"
#include "ui/theme/theme_manager.h"
#include "ui/workspace/page_registry.h"

namespace app {

AppServices::AppServices(const AppInfo& info, configs::AppSettings& settings,
                         ui::theme::ThemeManager& theme_manager,
                         ui::workspace::PageRegistry& page_registry,
                         commands::CommandRegistry& command_registry)
    : info_(info),
      settings_(settings),
      theme_manager_(theme_manager),
      page_registry_(page_registry),
      command_registry_(command_registry) {}

const AppInfo& AppServices::Info() const {
    return info_;
}

configs::AppSettings& AppServices::Settings() const {
    return settings_;
}

ui::theme::ThemeManager& AppServices::Theme() const {
    return theme_manager_;
}

ui::workspace::PageRegistry& AppServices::Pages() const {
    return page_registry_;
}

commands::CommandRegistry& AppServices::Commands() const {
    return command_registry_;
}

}  // namespace app
