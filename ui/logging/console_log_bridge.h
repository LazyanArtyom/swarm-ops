#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <memory>

#include "logging/log_store.h"
#include "logging/logger.h"

namespace app::ui {

class ConsoleOutputWidget;

namespace logging {

class ConsoleLogBridge final : public QObject {
    Q_OBJECT

   public:
    ConsoleLogBridge(ConsoleOutputWidget* output, std::shared_ptr<app::logging::LogStore> store,
                     app::logging::LogLevel minimum_level, QObject* parent = nullptr);
    ~ConsoleLogBridge() override;

    void ReloadFromStore();
    void Clear();
    void SetMinimumLevel(app::logging::LogLevel level);
    void SetSearchText(const QString& search_text);

    [[nodiscard]] app::logging::LogLevel MinimumLevel() const;
    [[nodiscard]] QString SearchText() const;

   signals:
    void SigRecord(app::logging::LogRecord record);
    void SigReload();

   private:
    [[nodiscard]] bool ShouldDisplay(const app::logging::LogRecord& record) const;
    [[nodiscard]] bool MatchesSearch(const app::logging::LogRecord& record) const;

    QPointer<ConsoleOutputWidget> output_;
    std::shared_ptr<app::logging::LogStore> store_;
    app::logging::LogStore::ObserverToken observer_token_{};
    app::logging::LogLevel minimum_level_{app::logging::LogLevel::kInfo};
    QString search_text_;
};

}  // namespace logging
}  // namespace app::ui

Q_DECLARE_METATYPE(app::logging::LogRecord)
