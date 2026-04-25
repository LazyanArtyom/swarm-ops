#include "app/commands/command.h"

#include <utility>

namespace app::commands {

bool CommandResult::Succeeded() const {
    return status == CommandStatus::kSuccess;
}

CommandResult CommandResult::Success(QString message) {
    return {.status = CommandStatus::kSuccess, .message = std::move(message)};
}

CommandResult CommandResult::Cancelled(QString message) {
    return {.status = CommandStatus::kCancelled, .message = std::move(message)};
}

CommandResult CommandResult::Unavailable(QString message) {
    return {.status = CommandStatus::kUnavailable, .message = std::move(message)};
}

CommandResult CommandResult::Failed(QString message) {
    return {.status = CommandStatus::kFailed, .message = std::move(message)};
}

CommandOperation::CommandOperation(QObject* parent) : QObject(parent) {}

void CommandOperation::Complete(CommandResult result) {
    if (finished_) {
        return;
    }

    finished_ = true;
    emit SigFinished(result);
}

bool CommandExecution::IsDeferred() const {
    return !operation.isNull();
}

CommandExecution CommandExecution::Completed(CommandResult result) {
    return {
        .result = std::move(result),
        .operation = nullptr,
    };
}

CommandExecution CommandExecution::Deferred(CommandOperation* operation, CommandResult result) {
    return {
        .result = std::move(result),
        .operation = operation,
    };
}

LambdaCommand::LambdaCommand(CommandMetadata metadata, ExecuteCallback execute,
                             EnabledCallback enabled)
    : metadata_(std::move(metadata)),
      execute_(std::move(execute)),
      enabled_(std::move(enabled)) {}

const CommandMetadata& LambdaCommand::Metadata() const {
    return metadata_;
}

bool LambdaCommand::IsEnabled(const CommandContext& context) const {
    return !enabled_ || enabled_(context);
}

CommandExecution LambdaCommand::Execute(const CommandContext& context) {
    if (!IsEnabled(context)) {
        return CommandExecution::Completed(
            CommandResult::Unavailable(QStringLiteral("Command is disabled.")));
    }

    if (!execute_) {
        return CommandExecution::Completed(
            CommandResult::Failed(QStringLiteral("Command has no handler.")));
    }

    return execute_(context);
}

}  // namespace app::commands
