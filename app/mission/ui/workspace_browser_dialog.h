#pragma once

#include <QDialog>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace app::mission {

enum class WorkspaceDialogMode {
    kOpen,
    kSave,
};

class WorkspaceBrowserDialog final : public QDialog {
    Q_OBJECT

   public:
    explicit WorkspaceBrowserDialog(WorkspaceDialogMode mode, QWidget* parent = nullptr);

    [[nodiscard]] QString StatusMessage() const;
    [[nodiscard]] bool PerformedOperation() const;

   private:
    void BuildUi();
    void RefreshWorkspaces();
    void UpdateButtonState();
    void OpenSelectedWorkspace();
    void SaveCurrentWorkspace();
    void ShowStatus(QString message, bool error = false);
    [[nodiscard]] QString SelectedWorkspaceId() const;

    WorkspaceDialogMode mode_{WorkspaceDialogMode::kOpen};
    QTableWidget* workspace_table_{nullptr};
    QLineEdit* name_edit_{nullptr};
    QCheckBox* shared_check_{nullptr};
    QLabel* status_label_{nullptr};
    QPushButton* open_button_{nullptr};
    QPushButton* save_button_{nullptr};
    QString status_message_;
    bool performed_operation_{false};
};

}  // namespace app::mission
