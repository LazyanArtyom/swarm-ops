#pragma once

#include <QObject>

class QAction;

namespace app::ui::actions {

class AppActions final : public QObject {
    Q_OBJECT

   public:
    explicit AppActions(QObject* parent = nullptr);
    ~AppActions() override = default;

    [[nodiscard]] QAction* NewAction() const;
    [[nodiscard]] QAction* OpenAction() const;
    [[nodiscard]] QAction* SaveAction() const;
    [[nodiscard]] QAction* SaveAsAction() const;
    [[nodiscard]] QAction* SettingsAction() const;
    [[nodiscard]] QAction* ExitAction() const;
    [[nodiscard]] QAction* ToggleNavigationAction() const;
    [[nodiscard]] QAction* ToggleInfoAction() const;
    [[nodiscard]] QAction* ToggleConsoleAction() const;
    [[nodiscard]] QAction* ResetLayoutAction() const;
    [[nodiscard]] QAction* AboutAction() const;

    void SetPanelVisibilityChecked(bool navigation_visible, bool info_visible,
                                   bool console_visible);

   private:
    void CreateActions();
    void BindIcons();
    void ConfigureToolTips();

    QAction* new_action_{nullptr};
    QAction* open_action_{nullptr};
    QAction* save_action_{nullptr};
    QAction* save_as_action_{nullptr};
    QAction* settings_action_{nullptr};
    QAction* exit_action_{nullptr};
    QAction* toggle_navigation_action_{nullptr};
    QAction* toggle_info_action_{nullptr};
    QAction* toggle_console_action_{nullptr};
    QAction* reset_layout_action_{nullptr};
    QAction* about_action_{nullptr};
};

}  // namespace app::ui::actions
