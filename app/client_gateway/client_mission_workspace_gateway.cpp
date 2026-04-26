#include "app/client_gateway/client_mission_workspace_gateway.h"

#include "app/client_gateway/mission_workspace_client.h"

namespace app::mission {

ClientMissionWorkspaceGateway::ClientMissionWorkspaceGateway(
    client_gateway::IMissionWorkspaceClient* client, QObject* parent)
    : IMissionWorkspaceGateway(parent), client_(client) {
    if (client_ == nullptr) {
        return;
    }

    client_->Start();
    active_workspace_ = client_->ActiveWorkspace();
    connect(client_, &client_gateway::IMissionWorkspaceClient::SigWorkspaceSnapshotChanged, this,
            [this](const MissionWorkspace& workspace,
                   client_gateway::WorkspaceUpdateType /*update_type*/) {
                active_workspace_ = workspace;
                emit SigWorkspaceChanged(active_workspace_);
            });
}

MissionWorkspace ClientMissionWorkspaceGateway::ActiveWorkspace() const {
    return active_workspace_;
}

QString ClientMissionWorkspaceGateway::CreateWorkspace(const WorkspaceBackground& background) {
    if (client_ == nullptr) {
        return {};
    }

    const QString workspace_id = client_->CreateWorkspace({}, background);
    if (!workspace_id.isEmpty()) {
        const auto join_result = client_->JoinWorkspaceSession(workspace_id, tr("Local Operator"));
        Q_UNUSED(join_result);
        active_workspace_ = client_->GetWorkspaceSnapshot(workspace_id);
    }
    return workspace_id;
}

void ClientMissionWorkspaceGateway::ReplaceWorkspace(MissionWorkspace workspace) {
    if (client_ == nullptr || workspace.id.isEmpty()) {
        return;
    }

    const auto result = client_->ApplyWorkspaceSnapshot(
        std::move(workspace), client_gateway::WorkspaceUpdateType::kOperationApplied);
    if (result.ok) {
        active_workspace_ = client_->ActiveWorkspace();
    }
}

}  // namespace app::mission
