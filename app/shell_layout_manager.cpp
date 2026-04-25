#include "app/shell_layout_manager.h"

#include <QDataStream>
#include <QGuiApplication>
#include <QIODevice>
#include <QMainWindow>
#include <QRect>
#include <QScreen>
#include <QSize>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <algorithm>
#include <optional>
#include <utility>

#include "core/configs/app_configs.h"
#include "ui/actions/app_actions.h"
#include "ui/panels/central_panel.h"
#include "ui/panels/console_panel.h"
#include "ui/panels/info_panel.h"
#include "ui/panels/navigation_panel.h"
#include "ui/theme/theme_metrics.h"

namespace app {
namespace {

constexpr auto kWorkspaceStateMagic = "workspace-v2";
constexpr auto kDefaultLayoutId = "default";
constexpr QDataStream::Version kWorkspaceStateStreamVersion = QDataStream::Qt_6_0;

struct WorkspaceState final {
    QByteArray horizontal_splitter;
    QByteArray vertical_splitter;
    bool navigation_visible{true};
    bool info_visible{true};
    bool console_visible{true};
};

QString NormalizedLayoutId(const QString& layout_id) {
    if (layout_id.trimmed().isEmpty()) {
        return QString::fromLatin1(kDefaultLayoutId);
    }
    return layout_id.trimmed();
}

void ConfigureWorkspaceSplitter(QSplitter* splitter) {
    if (splitter == nullptr) {
        return;
    }

    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(1);
    splitter->setOpaqueResize(true);
}

int BoundedDimension(int preferred, int minimum, int maximum) {
    if (maximum <= 0) {
        return preferred;
    }
    if (maximum < minimum) {
        return maximum;
    }
    return std::clamp(preferred, minimum, maximum);
}

QScreen* ScreenForWindow(const QMainWindow* window) {
    if (window != nullptr && window->screen() != nullptr) {
        return window->screen();
    }
    return QGuiApplication::primaryScreen();
}

QSize MinimumWindowSize(const ui::theme::ThemeMetricsData& metrics) {
    return {metrics.main_window_min_width_px, metrics.main_window_min_height_px};
}

QSize DefaultWindowSize(const QScreen* screen, const ui::theme::ThemeMetricsData& metrics) {
    const QSize preferred{
        metrics.main_window_default_width_px,
        metrics.main_window_default_height_px,
    };
    const QSize minimum = MinimumWindowSize(metrics);

    if (screen == nullptr) {
        return preferred;
    }

    const QRect available = screen->availableGeometry();
    const int max_width =
        static_cast<int>(available.width() * metrics.main_window_max_screen_width_ratio);
    const int max_height =
        static_cast<int>(available.height() * metrics.main_window_max_screen_height_ratio);

    return {BoundedDimension(preferred.width(), minimum.width(), max_width),
            BoundedDimension(preferred.height(), minimum.height(), max_height)};
}

void CenterOnScreen(QWidget* widget, const QScreen* screen) {
    if (widget == nullptr || screen == nullptr) {
        return;
    }

    const QRect available = screen->availableGeometry();
    QRect geometry(QPoint{}, widget->size());
    geometry.moveCenter(available.center());
    widget->move(geometry.topLeft());
}

QByteArray SerializeWorkspaceState(const WorkspaceState& state) {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(kWorkspaceStateStreamVersion);
    stream << QString::fromLatin1(kWorkspaceStateMagic);
    stream << state.horizontal_splitter;
    stream << state.vertical_splitter;
    stream << state.navigation_visible;
    stream << state.info_visible;
    stream << state.console_visible;
    return bytes;
}

std::optional<WorkspaceState> DeserializeWorkspaceState(const QByteArray& bytes) {
    QDataStream stream(bytes);
    stream.setVersion(kWorkspaceStateStreamVersion);

    QString magic;
    WorkspaceState state;
    stream >> magic >> state.horizontal_splitter >> state.vertical_splitter >>
        state.navigation_visible >> state.info_visible >> state.console_visible;

    if (stream.status() != QDataStream::Ok || magic != QString::fromLatin1(kWorkspaceStateMagic)) {
        return std::nullopt;
    }

    return state;
}

}  // namespace

ShellLayoutManager::ShellLayoutManager(QMainWindow* window, configs::AppSettings& settings,
                                       ui::actions::AppActions* app_actions,
                                       ui::CentralPanel* central_panel,
                                       MainWindowLayoutProfile layout_profile, QObject* parent)
    : QObject(parent),
      layout_profile_(std::move(layout_profile)),
      settings_(settings),
      window_(window),
      app_actions_(app_actions) {
    layout_profile_.id = NormalizedLayoutId(layout_profile_.id);
    widgets_.central_panel = central_panel;
}

void ShellLayoutManager::BuildShell() {
    if (window_ == nullptr || widgets_.central_panel == nullptr || widgets_.root != nullptr) {
        return;
    }

    widgets_.root = new QWidget(window_);
    auto* layout = new QVBoxLayout(widgets_.root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    widgets_.vertical_splitter = new QSplitter(Qt::Vertical, widgets_.root);
    ConfigureWorkspaceSplitter(widgets_.vertical_splitter);
    layout->addWidget(widgets_.vertical_splitter);

    widgets_.horizontal_splitter = new QSplitter(Qt::Horizontal, widgets_.vertical_splitter);
    ConfigureWorkspaceSplitter(widgets_.horizontal_splitter);

    widgets_.navigation_panel = new ui::NavigationPanel(widgets_.horizontal_splitter);
    widgets_.horizontal_splitter->addWidget(widgets_.navigation_panel);

    widgets_.horizontal_splitter->addWidget(widgets_.central_panel);

    widgets_.info_panel = new ui::InfoPanel(widgets_.horizontal_splitter);
    widgets_.horizontal_splitter->addWidget(widgets_.info_panel);

    widgets_.console_panel = new ui::ConsolePanel(widgets_.vertical_splitter);
    widgets_.vertical_splitter->addWidget(widgets_.horizontal_splitter);
    widgets_.vertical_splitter->addWidget(widgets_.console_panel);

    window_->setCentralWidget(widgets_.root);
    ApplyLayoutProfile(layout_profile_);
}

void ShellLayoutManager::Restore() {
    workspace_state_restored_ = false;
    page_session_restored_ = false;
    ApplyWindowConstraints();

    if (!RestoreWindowPlacement()) {
        ApplyDefaultWindowPlacement();
    }

    const QByteArray state = settings_.Window().LoadMainWindowLayoutState(layout_profile_.id);
    if (!state.isEmpty()) {
        workspace_state_restored_ = RestoreWorkspaceState(state);
    }
    const QByteArray page_session = settings_.Window().LoadPageSessionState(layout_profile_.id);
    if (!page_session.isEmpty()) {
        page_session_restored_ = RestorePageSessionState(page_session);
    }

    if (!workspace_state_restored_) {
        ApplyLayoutProfile(layout_profile_);
        SyncPanelVisibilityActions(layout_profile_.navigation_visible, layout_profile_.info_visible,
                                   layout_profile_.console_visible);
    }
}

void ShellLayoutManager::Save() const {
    if (window_ != nullptr) {
        settings_.Window().SaveMainWindowGeometry(window_->saveGeometry());
    }
    settings_.Window().SaveMainWindowLayoutState(layout_profile_.id, SaveWorkspaceState());
    settings_.Window().SavePageSessionState(layout_profile_.id, SavePageSessionState());
}

void ShellLayoutManager::Reset() {
    workspace_state_restored_ = false;
    page_session_restored_ = false;
    ApplyDefaultWindowPlacement();
    ApplyLayoutProfile(layout_profile_);
    SyncPanelVisibilityActions(layout_profile_.navigation_visible, layout_profile_.info_visible,
                               layout_profile_.console_visible);
    Save();
}

void ShellLayoutManager::AttachScreenTracking() {
    if ((screen_changed_connection_ != nullptr) || window_ == nullptr) {
        return;
    }

    if (QWindow* window_handle = window_->windowHandle(); window_handle != nullptr) {
        screen_changed_connection_ =
            connect(window_handle, &QWindow::screenChanged, this,
                    [this](QScreen*) { RebindTrackedScreen(); });
        RebindTrackedScreen();
    }
}

const ShellLayoutManager::ShellWidgets& ShellLayoutManager::Widgets() const {
    return widgets_;
}

bool ShellLayoutManager::HasRestoredPageSession() const {
    return page_session_restored_;
}

QByteArray ShellLayoutManager::SaveWorkspaceState() const {
    const WorkspaceState state{
        .horizontal_splitter =
            widgets_.horizontal_splitter != nullptr ? widgets_.horizontal_splitter->saveState()
                                                    : QByteArray(),
        .vertical_splitter =
            widgets_.vertical_splitter != nullptr ? widgets_.vertical_splitter->saveState()
                                                  : QByteArray(),
        .navigation_visible =
            widgets_.navigation_panel != nullptr && !widgets_.navigation_panel->isHidden(),
        .info_visible = widgets_.info_panel != nullptr && !widgets_.info_panel->isHidden(),
        .console_visible = widgets_.console_panel != nullptr && !widgets_.console_panel->isHidden(),
    };
    return SerializeWorkspaceState(state);
}

QByteArray ShellLayoutManager::SavePageSessionState() const {
    return widgets_.central_panel != nullptr ? widgets_.central_panel->SaveSession() : QByteArray();
}

bool ShellLayoutManager::RestoreWorkspaceState(const QByteArray& state) {
    const std::optional<WorkspaceState> layout = DeserializeWorkspaceState(state);
    if (!layout.has_value() || widgets_.horizontal_splitter == nullptr ||
        widgets_.vertical_splitter == nullptr) {
        return false;
    }

    const bool horizontal_ok =
        widgets_.horizontal_splitter->restoreState(layout->horizontal_splitter);
    const bool vertical_ok = widgets_.vertical_splitter->restoreState(layout->vertical_splitter);
    if (!horizontal_ok || !vertical_ok) {
        return false;
    }

    if (widgets_.navigation_panel != nullptr) {
        widgets_.navigation_panel->setVisible(layout->navigation_visible);
    }
    if (widgets_.info_panel != nullptr) {
        widgets_.info_panel->setVisible(layout->info_visible);
    }
    if (widgets_.console_panel != nullptr) {
        widgets_.console_panel->setVisible(layout->console_visible);
    }

    SyncPanelVisibilityActions(layout->navigation_visible, layout->info_visible,
                               layout->console_visible);
    return true;
}

bool ShellLayoutManager::RestorePageSessionState(const QByteArray& state) {
    return widgets_.central_panel != nullptr && widgets_.central_panel->RestoreSession(state);
}

bool ShellLayoutManager::RestoreWindowPlacement() {
    if (window_ == nullptr) {
        return false;
    }

    const QByteArray geometry = settings_.Window().LoadMainWindowGeometry();
    return !geometry.isEmpty() && window_->restoreGeometry(geometry);
}

void ShellLayoutManager::ApplyWindowConstraints() {
    if (window_ == nullptr) {
        return;
    }

    window_->setMinimumSize(MinimumWindowSize(ui::theme::ThemeMetrics::Instance().Current()));
}

void ShellLayoutManager::ApplyDefaultWindowPlacement() {
    if (window_ == nullptr) {
        return;
    }

    const QScreen* screen = ScreenForWindow(window_);
    window_->resize(DefaultWindowSize(screen, ui::theme::ThemeMetrics::Instance().Current()));
    CenterOnScreen(window_, screen);
}

void ShellLayoutManager::ApplyLayoutProfile(const MainWindowLayoutProfile& profile) const {
    const auto metrics = ui::theme::ThemeMetrics::Instance().Current();

    if (widgets_.navigation_panel != nullptr) {
        widgets_.navigation_panel->setMinimumWidth(metrics.navigation_min_width_px);
        widgets_.navigation_panel->setVisible(profile.navigation_visible);
    }
    if (widgets_.central_panel != nullptr) {
        widgets_.central_panel->setMinimumWidth(metrics.central_min_width_px);
    }
    if (widgets_.info_panel != nullptr) {
        widgets_.info_panel->setMinimumWidth(metrics.info_min_width_px);
        widgets_.info_panel->setVisible(profile.info_visible);
    }
    if (widgets_.console_panel != nullptr) {
        widgets_.console_panel->setMinimumHeight(metrics.console_min_height_px);
        widgets_.console_panel->setVisible(profile.console_visible);
    }

    if (widgets_.horizontal_splitter != nullptr) {
        widgets_.horizontal_splitter->setSizes(
            {profile.weights.navigation, profile.weights.central, profile.weights.info});
    }

    if (widgets_.vertical_splitter != nullptr) {
        widgets_.vertical_splitter->setSizes(
            {profile.weights.workspace, profile.weights.console});
    }
}

void ShellLayoutManager::RebindTrackedScreen() {
    QObject::disconnect(screen_dpi_connection_);

    if (window_ != nullptr) {
        if (QWindow* window_handle = window_->windowHandle(); window_handle != nullptr) {
            if (QScreen* screen = window_handle->screen(); screen != nullptr) {
                screen_dpi_connection_ =
                    connect(screen, &QScreen::logicalDotsPerInchChanged, this,
                            [](qreal) { ui::theme::ThemeMetrics::Instance().Refresh(); });
            }
        }
    }

    ui::theme::ThemeMetrics::Instance().Refresh();
    ApplyWindowConstraints();
    if (!workspace_state_restored_) {
        ApplyLayoutProfile(layout_profile_);
        SyncPanelVisibilityActions(layout_profile_.navigation_visible, layout_profile_.info_visible,
                                   layout_profile_.console_visible);
    }
}

void ShellLayoutManager::SyncPanelVisibilityActions(bool navigation_visible, bool info_visible,
                                                    bool console_visible) const {
    if (app_actions_ != nullptr) {
        app_actions_->SetPanelVisibilityChecked(navigation_visible, info_visible, console_visible);
    }
}

}  // namespace app
