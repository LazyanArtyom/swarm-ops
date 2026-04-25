#pragma once

#include <QTextEdit>
#include <QTimer>
#include <memory>
#include <mutex>
#include <vector>

#include "logging/logger.h"
#include "panels/panel_widget.h"

class QAction;
class QComboBox;
class QContextMenuEvent;
class QLineEdit;
class QToolButton;

namespace app::ui {

namespace logging {
class ConsoleLogBridge;
}  // namespace logging

/**
 * @brief Text widget that renders structured, colored console log messages.
 *
 * Features:
 *  - Buffered appends with a short flush interval to avoid UI thrash.
 *  - Per-level coloring and timestamped entries.
 *  - Buffered record rendering via an internal buffer + mutex.
 *  - Configurable maximum number of blocks in the QTextDocument.
 *
 * The widget is read-only and is intended to be fed by a UI-side logging bridge.
 */
class ConsoleOutputWidget : public QTextEdit {
    Q_OBJECT

   public:
    explicit ConsoleOutputWidget(QWidget* parent = nullptr);
    ConsoleOutputWidget(const ConsoleOutputWidget&) = delete;
    ConsoleOutputWidget& operator=(const ConsoleOutputWidget&) = delete;
    ~ConsoleOutputWidget() override = default;

    /**
     * @brief Appends a log record to the console buffer.
     *
     * This method must run on the GUI thread. Cross-thread delivery is handled by
     * the console logging bridge via a queued Qt signal.
     *
     * @param record Structured log record to render.
     */
    void AppendRecord(const app::logging::LogRecord& record);

    /**
     * @brief Clears the visible console and any buffered messages.
     */
    void ClearConsole();

    /**
     * @brief Returns true if there is no visible text and no pending buffer.
     */
    [[nodiscard]] bool IsEmpty() const;

   signals:
    void SigClearRequested();

   protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

   private slots:
    /**
     * @brief Flushes buffered log entries into the QTextDocument.
     *
     * This is invoked periodically by an internal QTimer on the GUI thread.
     */
    void FlushBuffer();

    /**
     * @brief Keeps the scroll bar anchored to the bottom after text changes.
     */
    void AdjustScrollBar();

   private:
    /**
     * @brief Renders a single log entry as HTML.
     */
    static QString ToHtml(const app::logging::LogRecord& record);

    QTimer flush_timer_;
    QAction* clear_action_{nullptr};
    std::vector<app::logging::LogRecord> buffer_;
    mutable std::mutex mutex_;
};

/**
 * @brief Simple panel that hosts a @ref ConsoleOutputWidget and subscribes it to the core logger.
 */
class ConsolePanel final : public PanelWidget {
    Q_OBJECT

   public:
    explicit ConsolePanel(QWidget* parent = nullptr);
    ~ConsolePanel() override;

    /**
     * @brief Returns the underlying output widget, never nullptr after construction.
     */
    [[nodiscard]] ConsoleOutputWidget* Output() const;

   signals:
    void SigCloseRequested();

   private slots:
    void OnLevelFilterChanged(int index);

   private:
    QWidget* CreateHeaderControls();
    void PopulateLevelFilter();
    void ClearLog();

    ConsoleOutputWidget* output_{nullptr};
    QComboBox* level_filter_{nullptr};
    QLineEdit* search_field_{nullptr};
    QToolButton* close_button_{nullptr};
    std::shared_ptr<logging::ConsoleLogBridge> log_bridge_;
};

}  // namespace app::ui
