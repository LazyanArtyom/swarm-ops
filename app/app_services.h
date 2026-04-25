#pragma once

#include "app/app_info.h"

namespace app::commands {
class CommandRegistry;
}

namespace app::configs {
class AppSettings;
}

namespace app::ui::theme {
class ThemeManager;
}

namespace app::ui::workspace {
class PageRegistry;
}

namespace app {

class AppServices final {
   public:
    AppServices(const AppInfo& info, configs::AppSettings& settings,
                ui::theme::ThemeManager& theme_manager, ui::workspace::PageRegistry& page_registry,
                commands::CommandRegistry& command_registry);

    [[nodiscard]] const AppInfo& Info() const;
    [[nodiscard]] configs::AppSettings& Settings() const;
    [[nodiscard]] ui::theme::ThemeManager& Theme() const;
    [[nodiscard]] ui::workspace::PageRegistry& Pages() const;
    [[nodiscard]] commands::CommandRegistry& Commands() const;

   private:
    const AppInfo& info_;
    configs::AppSettings& settings_;
    ui::theme::ThemeManager& theme_manager_;
    ui::workspace::PageRegistry& page_registry_;
    commands::CommandRegistry& command_registry_;
};

}  // namespace app
