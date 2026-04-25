#pragma once

#include <QMetaType>
#include <QObject>
#include <QPointer>
#include <QKeySequence>
#include <QString>
#include <cstdint>
#include <functional>

class QWidget;

namespace app {
class AppServices;
class DocumentSession;
}  // namespace app

namespace app::commands {

enum class CommandStatus : std::uint8_t {
    kSuccess,
    kCancelled,
    kUnavailable,
    kFailed,
};

struct CommandResult final {
    CommandStatus status{CommandStatus::kSuccess};
    QString message;

    [[nodiscard]] bool Succeeded() const;

    [[nodiscard]] static CommandResult Success(QString message = {});
    [[nodiscard]] static CommandResult Cancelled(QString message = {});
    [[nodiscard]] static CommandResult Unavailable(QString message = {});
    [[nodiscard]] static CommandResult Failed(QString message = {});
};

struct CommandPlacement final {
    QString menu_id;
    QString section_id;
    bool show_in_toolbar{false};
};

struct CommandMetadata final {
    QString command_id;
    QString title;
    QString description;
    QString group_id;
    QKeySequence shortcut;
    CommandPlacement placement;
};

struct CommandContext final {
    const app::AppServices* services{nullptr};
    app::DocumentSession* document_session{nullptr};
    QWidget* window{nullptr};
};

class CommandOperation final : public QObject {
    Q_OBJECT

   public:
    explicit CommandOperation(QObject* parent = nullptr);
    ~CommandOperation() override = default;

    void Complete(CommandResult result);

   signals:
    void SigFinished(const app::commands::CommandResult& result);

   private:
    bool finished_{false};
};

struct CommandExecution final {
    CommandResult result;
    QPointer<CommandOperation> operation;

    [[nodiscard]] bool IsDeferred() const;

    [[nodiscard]] static CommandExecution Completed(CommandResult result = CommandResult::Success());
    [[nodiscard]] static CommandExecution Deferred(CommandOperation* operation,
                                                  CommandResult result = CommandResult::Success());
};

class ICommand {
   public:
    virtual ~ICommand() = default;

    [[nodiscard]] virtual const CommandMetadata& Metadata() const = 0;
    [[nodiscard]] virtual bool IsEnabled(const CommandContext& context) const = 0;
    [[nodiscard]] virtual CommandExecution Execute(const CommandContext& context) = 0;
};

class LambdaCommand final : public ICommand {
   public:
    using EnabledCallback = std::function<bool(const CommandContext&)>;
    using ExecuteCallback = std::function<CommandExecution(const CommandContext&)>;

    LambdaCommand(CommandMetadata metadata, ExecuteCallback execute, EnabledCallback enabled = {});

    [[nodiscard]] const CommandMetadata& Metadata() const override;
    [[nodiscard]] bool IsEnabled(const CommandContext& context) const override;
    [[nodiscard]] CommandExecution Execute(const CommandContext& context) override;

   private:
    CommandMetadata metadata_;
    ExecuteCallback execute_;
    EnabledCallback enabled_;
};

}  // namespace app::commands

Q_DECLARE_METATYPE(app::commands::CommandResult)
