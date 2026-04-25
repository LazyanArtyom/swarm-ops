#pragma once

namespace app::commands {
class CommandRegistry;
}  // namespace app::commands

namespace app::ui::workspace {
class PageRegistry;
}  // namespace app::ui::workspace

namespace app::modules {

class IAppModule {
   public:
    virtual ~IAppModule() = default;

    virtual void RegisterPages(ui::workspace::PageRegistry& registry) = 0;
    virtual void RegisterCommands(commands::CommandRegistry& registry);
};

}  // namespace app::modules
