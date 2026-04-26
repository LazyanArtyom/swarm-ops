#include "app/mission/ui/mission_live_page.h"

#include <QAction>
#include <QIcon>
#include <QLabel>
#include <QSize>
#include <QToolBar>
#include <QVBoxLayout>

#include "app/client_gateway/client_gateway.h"
#include "app/client_gateway/swarm_runtime_client.h"
#include "app/mission/mission_workspace_service.h"
#include "app/mission/ui/simulation_page.h"
#include "logging/logger.h"
#include "ui/theme/theme_icons.h"
#include "ui/theme/theme_metrics.h"

namespace app::mission {
namespace {

constexpr auto kMissionLogCategory = "mission";
constexpr int kConsoleTelemetryFrameInterval = 30;

QString StateLabel(client_gateway::MissionSimulationState state) {
    switch (state) {
        case client_gateway::MissionSimulationState::kIdle:
            return QObject::tr("Idle");
        case client_gateway::MissionSimulationState::kRunning:
            return QObject::tr("Live");
        case client_gateway::MissionSimulationState::kPaused:
            return QObject::tr("Paused");
        case client_gateway::MissionSimulationState::kCompleted:
            return QObject::tr("Completed");
        case client_gateway::MissionSimulationState::kRejected:
            return QObject::tr("Rejected");
    }
    return QObject::tr("Unknown");
}

}  // namespace

MissionLivePage::MissionLivePage(QWidget* parent) : QWidget(parent) {
    const auto metrics = ui::theme::ThemeMetrics::Instance().Current();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tool_strip_ = new QToolBar(tr("Live Mission"), this);
    tool_strip_->setProperty("uiComponent", QStringLiteral("toolbar-chrome"));
    tool_strip_->setMovable(false);
    tool_strip_->setFloatable(false);
    tool_strip_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tool_strip_->setIconSize(QSize(metrics.icon_md_px, metrics.icon_md_px));

    stop_action_ = tool_strip_->addAction(tr("Stop"));
    tool_strip_->addSeparator();
    fit_action_ = tool_strip_->addAction(tr("Fit"));
    tool_strip_->addSeparator();

    status_label_ = new QLabel(tool_strip_);
    status_label_->setProperty("role", QStringLiteral("muted"));
    tool_strip_->addWidget(status_label_);
    layout->addWidget(tool_strip_);

    view_ = new SimulationView(this);
    layout->addWidget(view_, 1);

    ui::theme::ThemeIcons::Instance().BindAction(stop_action_, QStringLiteral("mission_sim_stop"));
    fit_action_->setIcon(QIcon(QStringLiteral(":/theme/icons/mission_fit.svg")));

    connect(stop_action_, &QAction::triggered, this, &MissionLivePage::StopMission);
    connect(fit_action_, &QAction::triggered, view_, &SimulationView::FitToWorkspace);
    connect(&MissionWorkspaceRuntime(), &MissionWorkspaceService::SigWorkspaceChanged, this,
            &MissionLivePage::OnWorkspaceChanged);
    connect(&client_gateway::ClientGatewayRuntime().SwarmRuntime(),
            &client_gateway::ISwarmRuntimeClient::SigLiveMissionFrame, this,
            &MissionLivePage::OnLiveMissionFrame);

    OnWorkspaceChanged(MissionWorkspaceRuntime().ActiveWorkspace());
    RefreshToolbarState(client_gateway::MissionSimulationState::kIdle);
    RefreshStatus(client_gateway::ClientGatewayRuntime().SwarmRuntime().LatestLiveMissionFrame());
}

void MissionLivePage::StopMission() {
    client_gateway::ClientGatewayRuntime().SwarmRuntime().StopLiveMission();
}

void MissionLivePage::OnWorkspaceChanged(const MissionWorkspace& workspace) {
    if (view_ != nullptr) {
        view_->SetWorkspace(workspace);
    }
}

void MissionLivePage::OnLiveMissionFrame(const client_gateway::MissionSimulationFrame& frame) {
    if (view_ != nullptr) {
        view_->ApplyFrame(frame);
    }

    RefreshToolbarState(frame.state);
    RefreshStatus(frame);

    if (!frame.message.isEmpty()) {
        const std::string message = frame.message.toStdString();
        logging::Logger::InfoFor(kMissionLogCategory, message);
    } else if (frame.frame_index % kConsoleTelemetryFrameInterval == 0) {
        logging::Logger::InfoFmtFor(
            kMissionLogCategory, "live telemetry frame={} drones={} segments={}",
            frame.frame_index, frame.drones.size(), frame.trail_segments.size());
    }
}

void MissionLivePage::RefreshToolbarState(client_gateway::MissionSimulationState state) {
    mission_state_ = state;
    stop_action_->setEnabled(state == client_gateway::MissionSimulationState::kRunning ||
                             state == client_gateway::MissionSimulationState::kPaused);
}

void MissionLivePage::RefreshStatus(const client_gateway::MissionSimulationFrame& frame) {
    if (status_label_ == nullptr) {
        return;
    }

    if (frame.workspace_id.isEmpty()) {
        status_label_->setText(tr("Mission telemetry idle"));
        return;
    }

    status_label_->setText(tr("State %1   Frame %2   Drones %3")
                               .arg(StateLabel(mission_state_))
                               .arg(frame.frame_index)
                               .arg(frame.drones.size()));
}

}  // namespace app::mission
