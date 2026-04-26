#include "app/mission/in_memory_mission_workspace_gateway.h"

#include <QDateTime>
#include <QUuid>
#include <utility>

namespace app::mission {

InMemoryMissionWorkspaceGateway::InMemoryMissionWorkspaceGateway(QObject* parent)
    : IMissionWorkspaceGateway(parent) {}

MissionWorkspace InMemoryMissionWorkspaceGateway::ActiveWorkspace() const {
    return workspaces_.value(active_workspace_id_);
}

QList<client_gateway::WorkspaceListItem> InMemoryMissionWorkspaceGateway::ListWorkspaces() const {
    QList<client_gateway::WorkspaceListItem> items;
    items.reserve(workspaces_.size());
    for (const MissionWorkspace& workspace : workspaces_) {
        items.push_back({
            .workspace_id = workspace.id,
            .name = workspace.name,
            .owner_display_name = tr("Local Operator"),
            .updated_at = QDateTime::currentDateTimeUtc(),
            .shared = false,
            .active_users = active_workspace_id_ == workspace.id ? 1 : 0,
        });
    }
    return items;
}

QString InMemoryMissionWorkspaceGateway::CreateWorkspace(const WorkspaceBackground& background) {
    MissionWorkspace workspace;
    workspace.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    workspace.name = tr("Mission %1").arg(next_workspace_number_++);
    workspace.background = background;

    active_workspace_id_ = workspace.id;
    workspaces_.insert(workspace.id, workspace);
    emit SigWorkspaceChanged(workspace);
    return workspace.id;
}

bool InMemoryMissionWorkspaceGateway::OpenWorkspace(const QString& workspace_id,
                                                    QString /*user_display_name*/,
                                                    QString* error_message) {
    if (!workspaces_.contains(workspace_id)) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace does not exist.");
        }
        return false;
    }

    active_workspace_id_ = workspace_id;
    emit SigWorkspaceChanged(workspaces_.value(workspace_id));
    return true;
}

bool InMemoryMissionWorkspaceGateway::SaveWorkspace(MissionWorkspace workspace,
                                                    QString* error_message) {
    if (workspace.id.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace id is empty.");
        }
        return false;
    }

    ReplaceWorkspace(std::move(workspace));
    return true;
}

bool InMemoryMissionWorkspaceGateway::SetWorkspaceShared(const QString& workspace_id,
                                                        bool /*shared*/,
                                                        QString* error_message) {
    if (!workspaces_.contains(workspace_id)) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace does not exist.");
        }
        return false;
    }
    return true;
}

client_gateway::WorkspaceShareInvite InMemoryMissionWorkspaceGateway::CreateShareInvite(
    const QString& workspace_id, QString* error_message) {
    client_gateway::WorkspaceShareInvite invite;
    if (!workspaces_.contains(workspace_id)) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace does not exist.");
        }
        return invite;
    }

    invite.workspace_id = workspace_id;
    invite.invite_code = QStringLiteral("LOCAL");
    invite.invite_url = QStringLiteral("swarmops://workspace/%1").arg(workspace_id);
    invite.expires_at = QDateTime::currentDateTimeUtc().addDays(7);
    return invite;
}

void InMemoryMissionWorkspaceGateway::ReplaceWorkspace(MissionWorkspace workspace) {
    if (workspace.id.isEmpty()) {
        return;
    }

    active_workspace_id_ = workspace.id;
    workspaces_.insert(workspace.id, workspace);
    emit SigWorkspaceChanged(workspace);
}

}  // namespace app::mission
