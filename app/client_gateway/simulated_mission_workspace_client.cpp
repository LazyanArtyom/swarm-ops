#include "app/client_gateway/simulated_mission_workspace_client.h"

#include <QUuid>
#include <algorithm>
#include <utility>

namespace app::client_gateway {
namespace {

QString NewId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QDateTime NowUtc() {
    return QDateTime::currentDateTimeUtc();
}

}  // namespace

SimulatedMissionWorkspaceClient::SimulatedMissionWorkspaceClient(QObject* parent)
    : IMissionWorkspaceClient(parent) {}

void SimulatedMissionWorkspaceClient::Start() {
    if (state_ == GatewayConnectionState::kConnected) {
        return;
    }

    state_ = GatewayConnectionState::kConnecting;
    emit SigConnectionChanged(state_);
    state_ = GatewayConnectionState::kConnected;
    emit SigConnectionChanged(state_);
}

void SimulatedMissionWorkspaceClient::Stop() {
    if (state_ == GatewayConnectionState::kDisconnected) {
        return;
    }

    LeaveWorkspaceSession();
    state_ = GatewayConnectionState::kDisconnected;
    emit SigConnectionChanged(state_);
}

GatewayConnectionState SimulatedMissionWorkspaceClient::ConnectionState() const {
    return state_;
}

QList<WorkspaceListItem> SimulatedMissionWorkspaceClient::ListWorkspaces() const {
    QList<WorkspaceListItem> items;
    items.reserve(workspaces_.size());
    for (const StoredWorkspace& stored : workspaces_) {
        items.push_back(ToListItem(stored));
    }
    std::sort(items.begin(), items.end(), [](const WorkspaceListItem& lhs,
                                             const WorkspaceListItem& rhs) {
        return lhs.updated_at > rhs.updated_at;
    });
    return items;
}

mission::MissionWorkspace SimulatedMissionWorkspaceClient::ActiveWorkspace() const {
    return workspaces_.value(active_workspace_id_).workspace;
}

mission::MissionWorkspace SimulatedMissionWorkspaceClient::GetWorkspaceSnapshot(
    const QString& workspace_id) const {
    return workspaces_.value(workspace_id).workspace;
}

QString SimulatedMissionWorkspaceClient::CreateWorkspace(
    QString name, const mission::WorkspaceBackground& background) {
    EnsureConnected();

    mission::MissionWorkspace workspace;
    workspace.id = NewId();
    workspace.name = name.trimmed().isEmpty() ? tr("Mission %1").arg(next_workspace_number_++)
                                              : name.trimmed();
    workspace.background = background;

    StoredWorkspace stored;
    stored.workspace = workspace;
    stored.owner_display_name = tr("Local Operator");
    stored.created_at = NowUtc();
    stored.updated_at = stored.created_at;
    stored.revision = next_revision_++;

    active_workspace_id_ = workspace.id;
    workspaces_.insert(workspace.id, stored);
    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged(workspace, WorkspaceUpdateType::kCreated);
    return workspace.id;
}

GatewayResult SimulatedMissionWorkspaceClient::SaveWorkspaceSnapshot(
    mission::MissionWorkspace workspace) {
    if (workspace.id.isEmpty()) {
        return GatewayResult::Failure(tr("Workspace id is empty."));
    }

    EnsureConnected();
    StoredWorkspace stored = workspaces_.value(workspace.id);
    if (stored.created_at.isNull()) {
        stored.created_at = NowUtc();
        stored.owner_display_name = tr("Local Operator");
        stored.shared = true;
    }
    stored.workspace = std::move(workspace);
    stored.updated_at = NowUtc();
    stored.revision = next_revision_++;
    active_workspace_id_ = stored.workspace.id;
    workspaces_.insert(stored.workspace.id, stored);

    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged(stored.workspace, WorkspaceUpdateType::kSnapshotSaved);
    return GatewayResult::Success(tr("Workspace saved in simulator."));
}

GatewayResult SimulatedMissionWorkspaceClient::DeleteWorkspace(const QString& workspace_id) {
    if (!workspaces_.contains(workspace_id)) {
        return GatewayResult::Failure(tr("Workspace does not exist."));
    }

    workspaces_.remove(workspace_id);
    checkpoints_.remove(workspace_id);
    presence_.remove(workspace_id);
    if (active_workspace_id_ == workspace_id) {
        active_workspace_id_.clear();
    }
    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged({}, WorkspaceUpdateType::kDeleted);
    return GatewayResult::Success(tr("Workspace deleted from simulator."));
}

GatewayResult SimulatedMissionWorkspaceClient::JoinWorkspaceSession(const QString& workspace_id,
                                                                    QString user_display_name) {
    if (!workspaces_.contains(workspace_id)) {
        return GatewayResult::Failure(tr("Workspace does not exist."));
    }

    active_workspace_id_ = workspace_id;
    WorkspacePresenceUser local_user;
    local_user.user_id = QStringLiteral("local-operator");
    local_user.display_name =
        user_display_name.trimmed().isEmpty() ? tr("Local Operator") : user_display_name.trimmed();
    local_user.joined_at = NowUtc();

    auto users = presence_.value(workspace_id);
    users.erase(std::remove_if(users.begin(), users.end(),
                               [&local_user](const WorkspacePresenceUser& user) {
                                   return user.user_id == local_user.user_id;
                               }),
                users.end());
    users.push_back(local_user);
    presence_.insert(workspace_id, users);

    EmitWorkspaceList();
    EmitPresence(workspace_id);
    emit SigWorkspaceSnapshotChanged(workspaces_.value(workspace_id).workspace,
                                     WorkspaceUpdateType::kSnapshotLoaded);
    return GatewayResult::Success(tr("Joined workspace session."));
}

void SimulatedMissionWorkspaceClient::LeaveWorkspaceSession() {
    if (active_workspace_id_.isEmpty()) {
        return;
    }

    const QString workspace_id = active_workspace_id_;
    active_workspace_id_.clear();
    EmitPresence(workspace_id);
}

GatewayResult SimulatedMissionWorkspaceClient::ApplyWorkspaceSnapshot(
    mission::MissionWorkspace workspace, WorkspaceUpdateType update_type) {
    if (workspace.id.isEmpty()) {
        return GatewayResult::Failure(tr("Workspace id is empty."));
    }

    EnsureConnected();
    StoredWorkspace stored = workspaces_.value(workspace.id);
    if (stored.created_at.isNull()) {
        stored.created_at = NowUtc();
        stored.owner_display_name = tr("Local Operator");
        stored.shared = true;
    }
    stored.workspace = std::move(workspace);
    stored.updated_at = NowUtc();
    stored.revision = next_revision_++;
    active_workspace_id_ = stored.workspace.id;
    workspaces_.insert(stored.workspace.id, stored);

    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged(stored.workspace, update_type);
    return GatewayResult::Success(tr("Workspace operation applied."));
}

WorkspaceCheckpoint SimulatedMissionWorkspaceClient::CreateCheckpoint(const QString& workspace_id,
                                                                      QString name) {
    WorkspaceCheckpoint checkpoint;
    if (!workspaces_.contains(workspace_id)) {
        return checkpoint;
    }

    checkpoint.checkpoint_id = NewId();
    checkpoint.workspace_id = workspace_id;
    checkpoint.name = name.trimmed().isEmpty() ? tr("Checkpoint") : name.trimmed();
    checkpoint.created_at = NowUtc();
    checkpoint.revision = workspaces_.value(workspace_id).revision;
    checkpoints_[workspace_id].push_back(checkpoint);
    return checkpoint;
}

WorkspaceListItem SimulatedMissionWorkspaceClient::ToListItem(
    const StoredWorkspace& stored) const {
    return {
        .workspace_id = stored.workspace.id,
        .name = stored.workspace.name,
        .owner_display_name = stored.owner_display_name,
        .updated_at = stored.updated_at,
        .shared = stored.shared,
        .active_users = static_cast<int>(presence_.value(stored.workspace.id).size()),
    };
}

QList<WorkspacePresenceUser> SimulatedMissionWorkspaceClient::PresenceFor(
    const QString& workspace_id) const {
    return presence_.value(workspace_id);
}

void SimulatedMissionWorkspaceClient::EmitWorkspaceList() {
    emit SigWorkspaceListChanged(ListWorkspaces());
}

void SimulatedMissionWorkspaceClient::EmitPresence(const QString& workspace_id) {
    emit SigWorkspacePresenceChanged(workspace_id, PresenceFor(workspace_id));
}

void SimulatedMissionWorkspaceClient::EnsureConnected() {
    if (state_ != GatewayConnectionState::kConnected) {
        Start();
    }
}

}  // namespace app::client_gateway
