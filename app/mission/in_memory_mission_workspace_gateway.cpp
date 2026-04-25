#include "app/mission/in_memory_mission_workspace_gateway.h"

#include <QUuid>

namespace app::mission {

InMemoryMissionWorkspaceGateway::InMemoryMissionWorkspaceGateway(QObject* parent)
    : IMissionWorkspaceGateway(parent) {}

MissionWorkspace InMemoryMissionWorkspaceGateway::ActiveWorkspace() const {
    return workspaces_.value(active_workspace_id_);
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

void InMemoryMissionWorkspaceGateway::ReplaceWorkspace(MissionWorkspace workspace) {
    if (workspace.id.isEmpty()) {
        return;
    }

    active_workspace_id_ = workspace.id;
    workspaces_.insert(workspace.id, workspace);
    emit SigWorkspaceChanged(workspace);
}

}  // namespace app::mission
