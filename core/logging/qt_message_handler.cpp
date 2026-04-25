#include "logging/qt_message_handler.h"

#include <QMessageLogContext>
#include <QString>
#include <QtGlobal>
#include <cstdlib>
#include <mutex>
#include <string>

#include "logging/logger.h"

namespace app::logging {
namespace {

constexpr auto kQtLogCategory = "qt";

struct QtMessageHandlerState final {
    std::mutex mutex;
    QtMessageHandler previous_handler{nullptr};
    bool installed{false};
};

[[nodiscard]] QtMessageHandlerState& HandlerState() {
    static QtMessageHandlerState state;
    return state;
}

[[nodiscard]] QtMessageHandler PreviousHandler() {
    QtMessageHandlerState& state = HandlerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    return state.previous_handler;
}

[[nodiscard]] std::string QtCategory(const QMessageLogContext& context) {
    if (context.category == nullptr || QString::fromLatin1(context.category).trimmed().isEmpty()) {
        return std::string{kQtLogCategory};
    }

    return QStringLiteral("%1.%2")
        .arg(QString::fromLatin1(kQtLogCategory), QString::fromLatin1(context.category))
        .toStdString();
}

void ForwardQtMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    const std::string category = QtCategory(context);
    const std::string text = message.toStdString();

    switch (type) {
        case QtDebugMsg:
            Logger::DebugFor(category, text);
            break;
        case QtInfoMsg:
            Logger::InfoFor(category, text);
            break;
        case QtWarningMsg:
            Logger::WarnFor(category, text);
            break;
        case QtCriticalMsg:
            Logger::ErrorFor(category, text);
            break;
        case QtFatalMsg:
            Logger::CriticalFor(category, text);
            Logger::Flush();
            if (const QtMessageHandler previous_handler = PreviousHandler();
                previous_handler != nullptr) {
                previous_handler(type, context, message);
            }
            std::abort();
    }
}

}  // namespace

void QtMessageHandlerBridge::Install() {
    QtMessageHandlerState& state = HandlerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.installed) {
        return;
    }

    state.previous_handler = qInstallMessageHandler(ForwardQtMessage);
    state.installed = true;
}

void QtMessageHandlerBridge::Uninstall() {
    QtMessageHandlerState& state = HandlerState();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.installed) {
        return;
    }

    qInstallMessageHandler(state.previous_handler);
    state.previous_handler = nullptr;
    state.installed = false;
}

}  // namespace app::logging
