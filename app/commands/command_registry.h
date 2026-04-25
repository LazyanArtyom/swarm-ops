#pragma once

#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <memory>

#include "app/commands/command.h"

namespace app::commands {

struct CommandDescriptor final {
    QString command_id;
    QString title;
    QString description;
    QString group_id;
    QKeySequence shortcut;
    CommandPlacement placement;
    bool enabled{true};
};

class CommandRegistry final {
   public:
    void RegisterCommand(std::shared_ptr<ICommand> command);
    void UnregisterCommand(const QString& command_id);

    [[nodiscard]] bool HasCommand(const QString& command_id) const;
    [[nodiscard]] bool IsCommandEnabled(const QString& command_id,
                                        const CommandContext& context) const;
    [[nodiscard]] std::shared_ptr<ICommand> FindCommand(const QString& command_id) const;
    [[nodiscard]] QList<CommandDescriptor> Commands(const CommandContext& context) const;
    [[nodiscard]] CommandExecution Execute(const QString& command_id,
                                           const CommandContext& context) const;

   private:
    QHash<QString, std::shared_ptr<ICommand>> commands_;
    QList<QString> order_;
};

}  // namespace app::commands
