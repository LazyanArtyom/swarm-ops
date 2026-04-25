#include "logging/console_log_bridge.h"

#include <QMetaType>
#include <QString>
#include <utility>

#include "panels/console_panel.h"

namespace app::ui::logging {
namespace {

QString FromStdString(std::string_view value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

}  // namespace

ConsoleLogBridge::ConsoleLogBridge(ConsoleOutputWidget* output,
                                   std::shared_ptr<app::logging::LogStore> store,
                                   app::logging::LogLevel minimum_level, QObject* parent)
    : QObject(parent),
      output_(output),
      store_(std::move(store)),
      minimum_level_(minimum_level) {
    qRegisterMetaType<app::logging::LogRecord>("app::logging::LogRecord");

    connect(
        this, &ConsoleLogBridge::SigRecord, this,
        [this](const app::logging::LogRecord& record) {
            if (output_ != nullptr) {
                output_->AppendRecord(record);
            }
        },
        Qt::QueuedConnection);
    connect(this, &ConsoleLogBridge::SigReload, this, &ConsoleLogBridge::ReloadFromStore,
            Qt::QueuedConnection);

    if (store_ != nullptr) {
        observer_token_ = store_->AddObserver([this](const app::logging::LogRecord& record) {
            if (ShouldDisplay(record)) {
                emit SigRecord(record);
            }
        }, true);
    }
}

ConsoleLogBridge::~ConsoleLogBridge() {
    if (store_ != nullptr) {
        store_->RemoveObserver(observer_token_);
    }
}

void ConsoleLogBridge::ReloadFromStore() {
    if (output_ == nullptr || store_ == nullptr) {
        return;
    }

    output_->ClearConsole();
    for (const auto& record : store_->Records(minimum_level_)) {
        if (MatchesSearch(record)) {
            output_->AppendRecord(record);
        }
    }
}

void ConsoleLogBridge::Clear() {
    if (store_ != nullptr) {
        store_->Clear();
    }
    if (output_ != nullptr) {
        output_->ClearConsole();
    }
}

void ConsoleLogBridge::SetMinimumLevel(app::logging::LogLevel level) {
    if (minimum_level_ == level) {
        return;
    }

    minimum_level_ = level;
    emit SigReload();
}

void ConsoleLogBridge::SetSearchText(const QString& search_text) {
    const QString normalized_search_text = search_text.trimmed();
    if (search_text_ == normalized_search_text) {
        return;
    }

    search_text_ = normalized_search_text;
    emit SigReload();
}

app::logging::LogLevel ConsoleLogBridge::MinimumLevel() const {
    return minimum_level_;
}

QString ConsoleLogBridge::SearchText() const {
    return search_text_;
}

bool ConsoleLogBridge::ShouldDisplay(const app::logging::LogRecord& record) const {
    return output_ != nullptr && app::logging::IsLevelEnabled(record.level, minimum_level_) &&
           MatchesSearch(record);
}

bool ConsoleLogBridge::MatchesSearch(const app::logging::LogRecord& record) const {
    if (search_text_.isEmpty()) {
        return true;
    }

    const QString category = FromStdString(record.category);
    const QString message = FromStdString(record.message);
    const QString level = FromStdString(app::logging::ToString(record.level));
    return category.contains(search_text_, Qt::CaseInsensitive) ||
           message.contains(search_text_, Qt::CaseInsensitive) ||
           level.contains(search_text_, Qt::CaseInsensitive);
}

}  // namespace app::ui::logging
