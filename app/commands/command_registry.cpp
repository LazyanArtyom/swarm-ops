#include "app/commands/command_registry.h"

#include <utility>

namespace app::commands {

void CommandRegistry::RegisterCommand(std::shared_ptr<ICommand> command) {
    if (command == nullptr || command->Metadata().command_id.isEmpty()) {
        return;
    }

    const QString command_id = command->Metadata().command_id;
    if (!commands_.contains(command_id)) {
        order_.push_back(command_id);
    }

    commands_[command_id] = std::move(command);
}

void CommandRegistry::UnregisterCommand(const QString& command_id) {
    commands_.remove(command_id);
    order_.removeAll(command_id);
}

bool CommandRegistry::HasCommand(const QString& command_id) const {
    return commands_.contains(command_id);
}

bool CommandRegistry::IsCommandEnabled(const QString& command_id,
                                       const CommandContext& context) const {
    const auto command = FindCommand(command_id);
    return command != nullptr && command->IsEnabled(context);
}

std::shared_ptr<ICommand> CommandRegistry::FindCommand(const QString& command_id) const {
    const auto command_it = commands_.find(command_id);
    if (command_it == commands_.end()) {
        return {};
    }

    return command_it.value();
}

QList<CommandDescriptor> CommandRegistry::Commands(const CommandContext& context) const {
    QList<CommandDescriptor> commands;
    commands.reserve(order_.size());

    for (const QString& command_id : order_) {
        const auto command = FindCommand(command_id);
        if (command != nullptr) {
            const CommandMetadata& metadata = command->Metadata();
            commands.push_back({
                .command_id = metadata.command_id,
                .title = metadata.title,
                .description = metadata.description,
                .group_id = metadata.group_id,
                .shortcut = metadata.shortcut,
                .placement = metadata.placement,
                .enabled = command->IsEnabled(context),
            });
        }
    }

    return commands;
}

CommandExecution CommandRegistry::Execute(const QString& command_id,
                                         const CommandContext& context) const {
    const auto command = FindCommand(command_id);
    if (command == nullptr) {
        return CommandExecution::Completed(CommandResult::Unavailable(
            QStringLiteral("Command '%1' is not registered.").arg(command_id)));
    }

    return command->Execute(context);
}

}  // namespace app::commands
