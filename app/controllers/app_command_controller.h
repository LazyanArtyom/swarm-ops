#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <vector>

#include "app/commands/command.h"

class QAction;
class QWidget;

namespace app::ui::actions {
class AppActions;
}  // namespace app::ui::actions

namespace app {
class AppServices;
class DocumentSession;
}  // namespace app

namespace app::controllers {

class AppCommandController final : public QObject {
    Q_OBJECT

   public:
    struct Targets final {
        const AppServices* services{nullptr};
        DocumentSession* document_session{nullptr};
        ui::actions::AppActions* app_actions{nullptr};
        QWidget* window{nullptr};
        QWidget* navigation_panel{nullptr};
        QWidget* info_panel{nullptr};
        QWidget* console_panel{nullptr};
    };

    explicit AppCommandController(Targets targets, QObject* parent = nullptr);
    ~AppCommandController() override = default;

    void Wire();

   private:
    void RegisterCommands();
    void WireActions();
    void BindActionToCommand(QAction* action, const QString& command_id);
    void UpdateCommandActionStates() const;
    [[nodiscard]] commands::CommandContext MakeCommandContext() const;

    [[nodiscard]] commands::CommandExecution ExecuteCommand(const QString& command_id) const;
    void TrackCommandExecution(const QString& command_id,
                               commands::CommandExecution execution) const;
    void HandleCommandResult(const QString& command_id,
                             const commands::CommandResult& result) const;

    [[nodiscard]] commands::CommandExecution OpenFile(const commands::CommandContext& context);
    [[nodiscard]] commands::CommandExecution SaveFile(const commands::CommandContext& context);
    [[nodiscard]] commands::CommandExecution SaveFileAs(const commands::CommandContext& context);

    Targets targets_;
    std::vector<std::shared_ptr<commands::ICommand>> registered_commands_;
    bool commands_registered_{false};
};

}  // namespace app::controllers
