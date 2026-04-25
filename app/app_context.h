#pragma once

#include <QObject>

#include "app/app_bootstrap.h"
#include "app/app_info.h"
#include "app/app_services.h"
#include "app/commands/command_registry.h"
#include "core/configs/app_configs.h"
#include "ui/theme/theme_manager.h"
#include "ui/workspace/page_registry.h"

namespace app::modules {
class IAppModule;
}  // namespace app::modules

namespace app {

struct AppContextDependencies final {
    configs::AppSettings* settings{nullptr};
    ui::theme::ThemeManager* theme_manager{nullptr};
    ui::workspace::PageRegistry* page_registry{nullptr};
    commands::CommandRegistry* command_registry{nullptr};
};

class AppContext final : public QObject {
    Q_OBJECT

   public:
    explicit AppContext(AppInfo info, QObject* parent = nullptr);
    AppContext(AppInfo info, AppContextDependencies dependencies, QObject* parent = nullptr);
    ~AppContext() override = default;

    [[nodiscard]] const AppServices& Services() const;
    [[nodiscard]] AppBootstrap& Bootstrap();
    [[nodiscard]] const AppBootstrap& Bootstrap() const;

   private:
    AppInfo info_;
    configs::AppSettings* settings_{nullptr};
    ui::theme::ThemeManager* theme_manager_{nullptr};
    ui::workspace::PageRegistry page_registry_storage_;
    commands::CommandRegistry command_registry_storage_;
    ui::workspace::PageRegistry* page_registry_{nullptr};
    commands::CommandRegistry* command_registry_{nullptr};
    AppServices services_;
    AppBootstrap bootstrap_;
};

}  // namespace app
