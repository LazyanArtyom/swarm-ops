#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QString>

#include "app/main_window_layout_profile.h"

class QMainWindow;
class QSplitter;
class QWidget;

namespace app::configs {
class AppSettings;
}

namespace app::ui {
class CentralPanel;
class ConsolePanel;
class InfoPanel;
class NavigationPanel;
namespace actions {
class AppActions;
}
}  // namespace app::ui

namespace app {

class ShellLayoutManager final : public QObject {
    Q_OBJECT

   public:
    struct ShellWidgets final {
        QWidget* root{nullptr};
        QSplitter* horizontal_splitter{nullptr};
        QSplitter* vertical_splitter{nullptr};
        ui::NavigationPanel* navigation_panel{nullptr};
        ui::CentralPanel* central_panel{nullptr};
        ui::InfoPanel* info_panel{nullptr};
        ui::ConsolePanel* console_panel{nullptr};
    };

    ShellLayoutManager(QMainWindow* window, configs::AppSettings& settings,
                       ui::actions::AppActions* app_actions, ui::CentralPanel* central_panel,
                       MainWindowLayoutProfile layout_profile = layout_profiles::Default(),
                       QObject* parent = nullptr);
    ~ShellLayoutManager() override = default;

    void BuildShell();
    void Restore();
    void Save() const;
    void Reset();
    void AttachScreenTracking();
    [[nodiscard]] bool HasRestoredPageSession() const;

    [[nodiscard]] const ShellWidgets& Widgets() const;

   private:
    [[nodiscard]] QByteArray SaveWorkspaceState() const;
    [[nodiscard]] QByteArray SavePageSessionState() const;
    [[nodiscard]] bool RestoreWorkspaceState(const QByteArray& state);
    [[nodiscard]] bool RestorePageSessionState(const QByteArray& state);
    [[nodiscard]] bool RestoreWindowPlacement();
    void ApplyWindowConstraints();
    void ApplyDefaultWindowPlacement();
    void ApplyLayoutProfile(const MainWindowLayoutProfile& profile) const;
    void RebindTrackedScreen();
    void SyncPanelVisibilityActions(bool navigation_visible, bool info_visible,
                                    bool console_visible) const;

    MainWindowLayoutProfile layout_profile_;
    configs::AppSettings& settings_;
    QMainWindow* window_{nullptr};
    ui::actions::AppActions* app_actions_{nullptr};
    ShellWidgets widgets_;
    bool workspace_state_restored_{false};
    bool page_session_restored_{false};
    QMetaObject::Connection screen_dpi_connection_;
    QMetaObject::Connection screen_changed_connection_;
};

}  // namespace app
