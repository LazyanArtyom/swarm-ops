#include "app/mission/ui/workspace_browser_dialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <utility>

#include "app/mission/mission_workspace_service.h"

namespace app::mission {
namespace {

constexpr int kWorkspaceIdRole = Qt::UserRole + 1;
constexpr int kNameColumn = 0;
constexpr int kOwnerColumn = 1;
constexpr int kSharedColumn = 2;
constexpr int kUsersColumn = 3;
constexpr int kUpdatedColumn = 4;
constexpr int kDialogMinWidthPx = 760;
constexpr int kDialogMinHeightPx = 460;
constexpr int kSaveDialogMinWidthPx = 520;
constexpr int kSaveDialogMinHeightPx = 220;

QTableWidgetItem* ReadOnlyItem(const QString& text) {
    auto* item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

}  // namespace

WorkspaceBrowserDialog::WorkspaceBrowserDialog(WorkspaceDialogMode mode, QWidget* parent)
    : QDialog(parent), mode_(mode) {
    BuildUi();
    if (mode_ == WorkspaceDialogMode::kOpen) {
        RefreshWorkspaces();
    }
    UpdateButtonState();
}

QString WorkspaceBrowserDialog::StatusMessage() const {
    return status_message_;
}

bool WorkspaceBrowserDialog::PerformedOperation() const {
    return performed_operation_;
}

void WorkspaceBrowserDialog::BuildUi() {
    setWindowTitle(mode_ == WorkspaceDialogMode::kOpen ? tr("Open Workspace")
                                                       : tr("Save Workspace"));
    setMinimumSize(mode_ == WorkspaceDialogMode::kOpen ? kDialogMinWidthPx
                                                       : kSaveDialogMinWidthPx,
                   mode_ == WorkspaceDialogMode::kOpen ? kDialogMinHeightPx
                                                       : kSaveDialogMinHeightPx);

    auto* layout = new QVBoxLayout(this);

    if (mode_ == WorkspaceDialogMode::kSave) {
        auto* name_label = new QLabel(tr("Workspace name"), this);
        layout->addWidget(name_label);

        name_edit_ = new QLineEdit(this);
        name_edit_->setText(MissionWorkspaceRuntime().WorkspaceDisplayName());
        name_edit_->selectAll();
        layout->addWidget(name_edit_);

        shared_check_ = new QCheckBox(tr("Share this workspace with everyone"), this);
        shared_check_->setToolTip(
            tr("Shared workspaces broadcast edits and mission telemetry to joined users."));
        layout->addWidget(shared_check_);
    } else {
        workspace_table_ = new QTableWidget(this);
        workspace_table_->setColumnCount(5);
        workspace_table_->setHorizontalHeaderLabels(
            {tr("Name"), tr("Owner"), tr("Shared"), tr("Users"), tr("Updated")});
        workspace_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
        workspace_table_->setSelectionMode(QAbstractItemView::SingleSelection);
        workspace_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        workspace_table_->setAlternatingRowColors(true);
        workspace_table_->verticalHeader()->setVisible(false);
        workspace_table_->horizontalHeader()->setStretchLastSection(true);
        workspace_table_->horizontalHeader()->setSectionResizeMode(kNameColumn, QHeaderView::Stretch);
        workspace_table_->horizontalHeader()->setSectionResizeMode(kOwnerColumn, QHeaderView::ResizeToContents);
        workspace_table_->horizontalHeader()->setSectionResizeMode(kSharedColumn, QHeaderView::ResizeToContents);
        workspace_table_->horizontalHeader()->setSectionResizeMode(kUsersColumn, QHeaderView::ResizeToContents);
        workspace_table_->horizontalHeader()->setSectionResizeMode(kUpdatedColumn, QHeaderView::ResizeToContents);
        layout->addWidget(workspace_table_, 1);
    }

    status_label_ = new QLabel(this);
    status_label_->setProperty("role", QStringLiteral("muted"));
    status_label_->setWordWrap(true);
    layout->addWidget(status_label_);

    auto* buttons = new QDialogButtonBox(this);
    if (mode_ == WorkspaceDialogMode::kSave) {
        save_button_ = buttons->addButton(tr("Save"), QDialogButtonBox::AcceptRole);
    } else {
        open_button_ = buttons->addButton(tr("Open Selected"), QDialogButtonBox::AcceptRole);
    }
    auto* close_button = buttons->addButton(QDialogButtonBox::Close);
    layout->addWidget(buttons);

    if (workspace_table_ != nullptr) {
        connect(workspace_table_, &QTableWidget::itemSelectionChanged, this,
                &WorkspaceBrowserDialog::UpdateButtonState);
        connect(workspace_table_, &QTableWidget::itemDoubleClicked, this,
                [this](QTableWidgetItem*) { OpenSelectedWorkspace(); });
    }
    if (save_button_ != nullptr) {
        connect(save_button_, &QPushButton::clicked, this,
                &WorkspaceBrowserDialog::SaveCurrentWorkspace);
    }
    if (open_button_ != nullptr) {
        connect(open_button_, &QPushButton::clicked, this,
                &WorkspaceBrowserDialog::OpenSelectedWorkspace);
    }
    connect(close_button, &QPushButton::clicked, this, &QDialog::reject);
}

void WorkspaceBrowserDialog::RefreshWorkspaces() {
    if (workspace_table_ == nullptr) {
        return;
    }

    const auto workspaces = MissionWorkspaceRuntime().AvailableWorkspaces();
    workspace_table_->setRowCount(workspaces.size());

    for (int row = 0; row < workspaces.size(); ++row) {
        const auto& workspace = workspaces[row];
        auto* name_item = ReadOnlyItem(workspace.name);
        name_item->setData(kWorkspaceIdRole, workspace.workspace_id);
        workspace_table_->setItem(row, kNameColumn, name_item);
        workspace_table_->setItem(row, kOwnerColumn, ReadOnlyItem(workspace.owner_display_name));
        workspace_table_->setItem(row, kSharedColumn,
                                  ReadOnlyItem(workspace.shared ? tr("Yes") : tr("No")));
        workspace_table_->setItem(row, kUsersColumn,
                                  ReadOnlyItem(QString::number(workspace.active_users)));
        workspace_table_->setItem(
            row, kUpdatedColumn,
            ReadOnlyItem(workspace.updated_at.toLocalTime().toString(Qt::ISODate)));
    }

    UpdateButtonState();
}

void WorkspaceBrowserDialog::UpdateButtonState() {
    if (open_button_ != nullptr) {
        open_button_->setEnabled(!SelectedWorkspaceId().isEmpty());
    }
    if (save_button_ != nullptr) {
        save_button_->setEnabled(MissionWorkspaceRuntime().HasActiveWorkspace());
    }
}

void WorkspaceBrowserDialog::OpenSelectedWorkspace() {
    const QString workspace_id = SelectedWorkspaceId();
    if (workspace_id.isEmpty()) {
        ShowStatus(tr("Select a workspace first."), true);
        return;
    }

    QString error_message;
    if (!MissionWorkspaceRuntime().OpenWorkspaceSession(workspace_id, tr("Local Operator"),
                                                        &error_message)) {
        ShowStatus(error_message, true);
        return;
    }

    performed_operation_ = true;
    status_message_ = tr("Opened workspace.");
    accept();
}

void WorkspaceBrowserDialog::SaveCurrentWorkspace() {
    const QString workspace_name = name_edit_->text().trimmed();
    if (workspace_name.isEmpty() || workspace_name == tr("Untitled")) {
        ShowStatus(tr("Enter a workspace name before saving."), true);
        name_edit_->setFocus();
        name_edit_->selectAll();
        return;
    }

    QString error_message;
    if (!MissionWorkspaceRuntime().SaveWorkspaceToGateway(workspace_name, shared_check_->isChecked(),
                                                          &error_message)) {
        ShowStatus(error_message, true);
        return;
    }

    performed_operation_ = true;
    ShowStatus(tr("Workspace saved."));
    if (shared_check_->isChecked()) {
        status_message_ = tr("Workspace saved and shared.");
    }
    accept();
}

void WorkspaceBrowserDialog::ShowStatus(QString message, bool error) {
    status_message_ = std::move(message);
    status_label_->setProperty("status", error ? QStringLiteral("error") : QStringLiteral("info"));
    status_label_->setText(status_message_);
    status_label_->style()->unpolish(status_label_);
    status_label_->style()->polish(status_label_);
}

QString WorkspaceBrowserDialog::SelectedWorkspaceId() const {
    if (workspace_table_ == nullptr) {
        return {};
    }

    const auto ranges = workspace_table_->selectedRanges();
    if (ranges.isEmpty()) {
        return {};
    }

    const int row = ranges.first().topRow();
    const auto* item = workspace_table_->item(row, kNameColumn);
    return item != nullptr ? item->data(kWorkspaceIdRole).toString() : QString();
}

}  // namespace app::mission
