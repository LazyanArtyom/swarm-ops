#include "app/client_gateway/client_mission_workspace_gateway.h"

#include <utility>

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
    connect(client_, &client_gateway::IMissionWorkspaceClient::SigWorkspaceListChanged, this,
            &ClientMissionWorkspaceGateway::SigWorkspaceListChanged);
    connect(client_, &client_gateway::IMissionWorkspaceClient::SigWorkspacePresenceChanged, this,
            &ClientMissionWorkspaceGateway::SigWorkspacePresenceChanged);
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

QList<client_gateway::WorkspaceListItem> ClientMissionWorkspaceGateway::ListWorkspaces() const {
    return client_ != nullptr ? client_->ListWorkspaces()
                              : QList<client_gateway::WorkspaceListItem>{};
}

QString ClientMissionWorkspaceGateway::CreateWorkspace(const WorkspaceBackground& background) {
    if (client_ == nullptr) {
        return {};
    }

    const QString workspace_id = client_->CreateWorkspace(tr("Untitled"), background);
    if (!workspace_id.isEmpty()) {
        const auto join_result = client_->JoinWorkspaceSession(workspace_id, tr("Local Operator"));
        Q_UNUSED(join_result);
        active_workspace_ = client_->GetWorkspaceSnapshot(workspace_id);
    }
    return workspace_id;
}

bool ClientMissionWorkspaceGateway::OpenWorkspace(const QString& workspace_id,
                                                  QString user_display_name,
                                                  QString* error_message) {
    if (client_ == nullptr || workspace_id.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace gateway is unavailable.");
        }
        return false;
    }

    const auto result = client_->JoinWorkspaceSession(workspace_id, std::move(user_display_name));
    if (!result.ok) {
        if (error_message != nullptr) {
            *error_message = result.message;
        }
        return false;
    }

    active_workspace_ = client_->GetWorkspaceSnapshot(workspace_id);
    emit SigWorkspaceChanged(active_workspace_);
    return true;
}

bool ClientMissionWorkspaceGateway::SaveWorkspace(MissionWorkspace workspace,
                                                  QString* error_message) {
    if (client_ == nullptr || workspace.id.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace gateway is unavailable.");
        }
        return false;
    }

    const auto result = client_->SaveWorkspaceSnapshot(std::move(workspace));
    if (!result.ok) {
        if (error_message != nullptr) {
            *error_message = result.message;
        }
        return false;
    }

    active_workspace_ = client_->ActiveWorkspace();
    return true;
}

bool ClientMissionWorkspaceGateway::SetWorkspaceShared(const QString& workspace_id, bool shared,
                                                       QString* error_message) {
    if (client_ == nullptr || workspace_id.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace gateway is unavailable.");
        }
        return false;
    }

    const auto result = client_->SetWorkspaceShared(workspace_id, shared);
    if (!result.ok) {
        if (error_message != nullptr) {
            *error_message = result.message;
        }
        return false;
    }

    return true;
}

client_gateway::WorkspaceShareInvite ClientMissionWorkspaceGateway::CreateShareInvite(
    const QString& workspace_id, QString* error_message) {
    client_gateway::WorkspaceShareInvite invite;
    if (client_ == nullptr || workspace_id.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace gateway is unavailable.");
        }
        return invite;
    }

    invite = client_->CreateWorkspaceInvite(workspace_id);
    if (invite.workspace_id.isEmpty() && error_message != nullptr) {
        *error_message = tr("Could not create a workspace share invite.");
    }
    return invite;
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
