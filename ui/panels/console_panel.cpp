#include "panels/console_panel.h"

#include <QAction>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QSignalBlocker>
#include <QStyle>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QToolButton>
#include <QVBoxLayout>
#include <optional>
#include <chrono>

#include "logging/console_log_bridge.h"
#include "object_names.h"
#include "theme/theme_manager.h"
#include "theme/theme_metrics.h"

namespace app::ui {
namespace {

// Buffer flush interval (ms).
constexpr int kFlushIntervalMs = 250;
// Max number of text blocks stored in the document.
constexpr int kMaxConsoleBlocks = 8000;
// Timestamp format used in log lines.
constexpr auto kTimestampFormat = "HH:mm:ss.zzz";
constexpr auto kClearLogShortcut = "Ctrl+L";
constexpr auto kConsoleLogCategory = "ui.console";
QString FromStdString(std::string_view value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

QString LevelLabel(app::logging::LogLevel level) {
    switch (level) {
        case app::logging::LogLevel::kTrace:
            return QCoreApplication::translate("app::ui::ConsoleOutputWidget", "TRACE");
        case app::logging::LogLevel::kDebug:
            return QCoreApplication::translate("app::ui::ConsoleOutputWidget", "DEBUG");
        case app::logging::LogLevel::kInfo:
            return QCoreApplication::translate("app::ui::ConsoleOutputWidget", "INFO");
        case app::logging::LogLevel::kWarn:
            return QCoreApplication::translate("app::ui::ConsoleOutputWidget", "WARN");
        case app::logging::LogLevel::kError:
            return QCoreApplication::translate("app::ui::ConsoleOutputWidget", "ERROR");
        case app::logging::LogLevel::kCritical:
            return QCoreApplication::translate("app::ui::ConsoleOutputWidget", "CRITICAL");
        case app::logging::LogLevel::kOff:
            return QCoreApplication::translate("app::ui::ConsoleOutputWidget", "OFF");
    }
    return QCoreApplication::translate("app::ui::ConsoleOutputWidget", "INFO");
}

std::optional<app::logging::LogLevel> LevelFromValue(int value) {
    const auto level = static_cast<app::logging::LogLevel>(value);
    switch (level) {
        case app::logging::LogLevel::kTrace:
        case app::logging::LogLevel::kDebug:
        case app::logging::LogLevel::kInfo:
        case app::logging::LogLevel::kWarn:
        case app::logging::LogLevel::kError:
        case app::logging::LogLevel::kCritical:
        case app::logging::LogLevel::kOff:
            return level;
    }
    return std::nullopt;
}

QString LevelColor(app::logging::LogLevel level) {
    const theme::ThemeColors colors = theme::ThemeManager::Instance().CurrentColors();

    switch (level) {
        case app::logging::LogLevel::kTrace:
        case app::logging::LogLevel::kDebug:
            return colors.fg_muted.name();
        case app::logging::LogLevel::kInfo:
            return colors.info.name();
        case app::logging::LogLevel::kWarn:
            return colors.warning.name();
        case app::logging::LogLevel::kError:
        case app::logging::LogLevel::kCritical:
            return colors.error.name();
        case app::logging::LogLevel::kOff:
            return colors.fg_muted.name();
    }
    return colors.info.name();
}

QString MetadataColor() {
    return theme::ThemeManager::Instance().CurrentColors().console_metadata.name();
}

QString FormatTimestamp(std::chrono::system_clock::time_point timestamp) {
    const auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count();
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(millis))
        .toString(QString::fromLatin1(kTimestampFormat));
}

QIcon ClearLogActionIcon(QWidget* widget) {
    return QIcon::fromTheme(
        QStringLiteral("edit-clear-history"),
        widget != nullptr ? widget->style()->standardIcon(QStyle::SP_DialogResetButton) : QIcon());
}

QIcon CopyActionIcon(QWidget* widget) {
    return QIcon::fromTheme(
        QStringLiteral("edit-copy"),
        widget != nullptr ? widget->style()->standardIcon(QStyle::SP_FileIcon) : QIcon());
}

QIcon SelectAllActionIcon(QWidget* widget) {
    return QIcon::fromTheme(
        QStringLiteral("edit-select-all"),
        widget != nullptr ? widget->style()->standardIcon(QStyle::SP_DialogApplyButton) : QIcon());
}

void SetPointingHandCursor(QWidget* widget) {
    if (widget != nullptr) {
        widget->setCursor(Qt::PointingHandCursor);
    }
}

void ShowShortcutInContextMenu(QAction* action, const QKeySequence& shortcut) {
    if (action == nullptr) {
        return;
    }

    action->setShortcut(shortcut);
    action->setShortcutVisibleInContextMenu(true);
}

}  // namespace

ConsoleOutputWidget::ConsoleOutputWidget(QWidget* parent) : QTextEdit(parent), flush_timer_(this) {
    setObjectName(QString::fromLatin1(object_names::kConsoleOutput));

    setReadOnly(true);
    setUndoRedoEnabled(false);
    setWordWrapMode(QTextOption::NoWrap);

    if (auto* doc = document()) {
        doc->setMaximumBlockCount(kMaxConsoleBlocks);
    }

    connect(this, &QTextEdit::textChanged, this, &ConsoleOutputWidget::AdjustScrollBar);

    flush_timer_.setInterval(kFlushIntervalMs);
    flush_timer_.setSingleShot(true);
    connect(&flush_timer_, &QTimer::timeout, this, &ConsoleOutputWidget::FlushBuffer);

    clear_action_ = new QAction(ClearLogActionIcon(this), tr("Clear Log"), this);
    clear_action_->setShortcut(QKeySequence(QString::fromLatin1(kClearLogShortcut)));
    clear_action_->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    clear_action_->setShortcutVisibleInContextMenu(true);
    connect(clear_action_, &QAction::triggered, this, &ConsoleOutputWidget::SigClearRequested);
    addAction(clear_action_);
}

void ConsoleOutputWidget::AppendRecord(const app::logging::LogRecord& record) {
    if (record.message.empty()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_.push_back(record);
    }

    if (!flush_timer_.isActive()) {
        flush_timer_.start();
    }
}

void ConsoleOutputWidget::ClearConsole() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_.clear();
    }
    flush_timer_.stop();
    clear();
}

bool ConsoleOutputWidget::IsEmpty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto* doc = document();
    return buffer_.empty() && (doc == nullptr || doc->isEmpty());
}

QString ConsoleOutputWidget::ToHtml(const app::logging::LogRecord& record) {
    QString escaped = FromStdString(record.message).toHtmlEscaped();
    escaped.replace(QStringLiteral("\t"), QStringLiteral("    "));
    escaped.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));

    const QString label = LevelLabel(record.level);
    const QString level_color = LevelColor(record.level);
    const QString metadata_color = MetadataColor();
    const QString category = FromStdString(record.category).toHtmlEscaped();
    const QString timestamp = FormatTimestamp(record.timestamp);

    // No font-family/background hardcoded here; base styling comes from QSS.
    return QStringLiteral(
               "<div>"
               "<span style=\"color:%1;\">[%2]</span> "
               "<span style=\"color:%3; font-weight:600;\">[%4]</span> "
               "<span style=\"color:%1;\">[%5]</span> "
               "<span>%6</span>"
               "</div>")
        .arg(metadata_color, timestamp, level_color, label, category, escaped);
}

void ConsoleOutputWidget::FlushBuffer() {
    std::vector<app::logging::LogRecord> local_buffer;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (buffer_.empty()) {
            return;
        }
        local_buffer.swap(buffer_);
    }

    auto* doc = document();
    if (doc == nullptr) {
        return;
    }

    setUpdatesEnabled(false);
    doc->blockSignals(true);

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();

    for (const auto& chunk : local_buffer) {
        cursor.insertHtml(ToHtml(chunk));
        cursor.insertBlock();
    }

    cursor.endEditBlock();
    doc->blockSignals(false);
    setUpdatesEnabled(true);

    setTextCursor(cursor);
    ensureCursorVisible();
}

void ConsoleOutputWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);

    QAction* copy_action = menu.addAction(CopyActionIcon(this), tr("Copy"));
    copy_action->setEnabled(textCursor().hasSelection());
    ShowShortcutInContextMenu(copy_action, QKeySequence::Copy);
    connect(copy_action, &QAction::triggered, this, &ConsoleOutputWidget::copy);

    QAction* select_all_action = menu.addAction(SelectAllActionIcon(this), tr("Select All"));
    select_all_action->setEnabled(!IsEmpty());
    ShowShortcutInContextMenu(select_all_action, QKeySequence::SelectAll);
    connect(select_all_action, &QAction::triggered, this, &ConsoleOutputWidget::selectAll);

    menu.addSeparator();
    QAction* clear_log = menu.addAction(ClearLogActionIcon(this), tr("Clear Log"));
    clear_log->setEnabled(!IsEmpty());
    ShowShortcutInContextMenu(clear_log, QKeySequence(QString::fromLatin1(kClearLogShortcut)));
    connect(clear_log, &QAction::triggered, this, &ConsoleOutputWidget::SigClearRequested);
    menu.exec(event->globalPos());
}

void ConsoleOutputWidget::AdjustScrollBar() {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    ensureCursorVisible();
}

ConsolePanel::ConsolePanel(QWidget* parent)
    : PanelWidget(tr("Console"), object_names::kConsolePanel, parent) {
    const auto metrics = theme::ThemeMetrics::Instance().Current();

    auto* layout = ContentLayout();
    layout->setContentsMargins(metrics.spacing_sm_px, metrics.spacing_sm_px, metrics.spacing_sm_px,
                               metrics.spacing_sm_px);
    layout->setSpacing(metrics.spacing_sm_px);

    AddHeaderWidget(CreateHeaderControls());

    output_ = new ConsoleOutputWidget(ContentWidget());
    layout->addWidget(output_, 1);

    log_bridge_ = std::make_shared<logging::ConsoleLogBridge>(
        output_, app::logging::Logger::Store(), app::logging::Logger::CurrentConfig().console_level);

    PopulateLevelFilter();

    connect(output_, &ConsoleOutputWidget::SigClearRequested, this, &ConsolePanel::ClearLog);
    connect(search_field_, &QLineEdit::textChanged, this, [this](const QString& search_text) {
        if (log_bridge_ != nullptr) {
            log_bridge_->SetSearchText(search_text);
        }
    });
}

ConsolePanel::~ConsolePanel() = default;

ConsoleOutputWidget* ConsolePanel::Output() const {
    return output_;
}

void ConsolePanel::OnLevelFilterChanged(int index) {
    if (level_filter_ == nullptr || log_bridge_ == nullptr) {
        return;
    }

    const std::optional<app::logging::LogLevel> level =
        LevelFromValue(level_filter_->itemData(index).toInt());
    if (!level.has_value()) {
        app::logging::Logger::WarnFmtFor(kConsoleLogCategory,
                                         "Ignored invalid console level filter value: {}",
                                         level_filter_->itemData(index).toInt());
        return;
    }

    log_bridge_->SetMinimumLevel(*level);
}

QWidget* ConsolePanel::CreateHeaderControls() {
    auto* controls = new QWidget(this);
    controls->setObjectName(QString::fromLatin1(object_names::kConsoleHeaderControls));
    controls->setProperty("uiComponent", QStringLiteral("console-header-controls"));

    const auto metrics = theme::ThemeMetrics::Instance().Current();
    auto* controls_layout = new QHBoxLayout(controls);
    controls_layout->setContentsMargins(0, 0, 0, 0);
    controls_layout->setSpacing(metrics.spacing_xs_px);

    level_filter_ = new QComboBox(controls);
    level_filter_->setObjectName(QString::fromLatin1(object_names::kConsoleLevelFilter));
    level_filter_->setProperty("consoleHeaderControl", true);
    level_filter_->setMinimumWidth(metrics.console_level_filter_min_width_px);
    level_filter_->setFixedHeight(metrics.console_header_control_height_px);
    level_filter_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    level_filter_->setToolTip(tr("Minimum level"));
    connect(level_filter_, &QComboBox::currentIndexChanged, this,
            &ConsolePanel::OnLevelFilterChanged);

    search_field_ = new QLineEdit(controls);
    search_field_->setObjectName(QString::fromLatin1(object_names::kConsoleSearch));
    search_field_->setProperty("consoleHeaderControl", true);
    search_field_->setClearButtonEnabled(true);
    search_field_->setPlaceholderText(tr("Search logs"));
    search_field_->setMinimumWidth(metrics.console_search_min_width_px);
    search_field_->setMaximumWidth(metrics.console_search_max_width_px);
    search_field_->setFixedHeight(metrics.console_header_control_height_px);

    close_button_ = new QToolButton(controls);
    close_button_->setObjectName(QString::fromLatin1(object_names::kPanelCloseButton));
    close_button_->setText(QStringLiteral("x"));
    close_button_->setToolTip(tr("Close console"));
    close_button_->setFixedSize(metrics.panel_close_button_extent_px,
                                metrics.panel_close_button_extent_px);
    SetPointingHandCursor(close_button_);
    connect(close_button_, &QToolButton::clicked, this, &ConsolePanel::SigCloseRequested);

    controls_layout->addWidget(level_filter_);
    controls_layout->addWidget(search_field_);
    controls_layout->addWidget(close_button_);
    return controls;
}

void ConsolePanel::PopulateLevelFilter() {
    if (level_filter_ == nullptr || log_bridge_ == nullptr) {
        return;
    }

    const QSignalBlocker blocker(level_filter_);
    level_filter_->clear();
    level_filter_->addItem(tr("Trace"), static_cast<int>(app::logging::LogLevel::kTrace));
    level_filter_->addItem(tr("Debug"), static_cast<int>(app::logging::LogLevel::kDebug));
    level_filter_->addItem(tr("Info"), static_cast<int>(app::logging::LogLevel::kInfo));
    level_filter_->addItem(tr("Warn"), static_cast<int>(app::logging::LogLevel::kWarn));
    level_filter_->addItem(tr("Error"), static_cast<int>(app::logging::LogLevel::kError));
    level_filter_->addItem(tr("Critical"), static_cast<int>(app::logging::LogLevel::kCritical));

    const int configured_index =
        level_filter_->findData(static_cast<int>(log_bridge_->MinimumLevel()));
    if (configured_index >= 0) {
        level_filter_->setCurrentIndex(configured_index);
    }
}

void ConsolePanel::ClearLog() {
    if (log_bridge_ != nullptr) {
        log_bridge_->Clear();
        return;
    }
    if (output_ != nullptr) {
        output_->ClearConsole();
    }
}

}  // namespace app::ui
