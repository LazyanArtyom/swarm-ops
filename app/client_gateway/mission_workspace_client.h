#pragma once

#include <QObject>
#include <QString>

#include "app/client_gateway/client_gateway_types.h"

namespace app::client_gateway {

class IMissionWorkspaceClient : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;
    ~IMissionWorkspaceClient() override = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    [[nodiscard]] virtual GatewayConnectionState ConnectionState() const = 0;

    [[nodiscard]] virtual QList<WorkspaceListItem> ListWorkspaces() const = 0;
    [[nodiscard]] virtual mission::MissionWorkspace ActiveWorkspace() const = 0;
    [[nodiscard]] virtual mission::MissionWorkspace GetWorkspaceSnapshot(
        const QString& workspace_id) const = 0;

    [[nodiscard]] virtual QString CreateWorkspace(QString name,
                                                  const mission::WorkspaceBackground& background) = 0;
    [[nodiscard]] virtual GatewayResult SaveWorkspaceSnapshot(
        mission::MissionWorkspace workspace) = 0;
    [[nodiscard]] virtual GatewayResult DeleteWorkspace(const QString& workspace_id) = 0;

    [[nodiscard]] virtual GatewayResult JoinWorkspaceSession(const QString& workspace_id,
                                                             QString user_display_name) = 0;
    virtual void LeaveWorkspaceSession() = 0;
    [[nodiscard]] virtual GatewayResult ApplyWorkspaceSnapshot(
        mission::MissionWorkspace workspace, WorkspaceUpdateType update_type) = 0;

    [[nodiscard]] virtual WorkspaceCheckpoint CreateCheckpoint(const QString& workspace_id,
                                                               QString name) = 0;

   signals:
    void SigConnectionChanged(app::client_gateway::GatewayConnectionState state);
    void SigWorkspaceListChanged(QList<app::client_gateway::WorkspaceListItem> workspaces);
    void SigWorkspaceSnapshotChanged(const app::mission::MissionWorkspace& workspace,
                                     app::client_gateway::WorkspaceUpdateType update_type);
    void SigWorkspacePresenceChanged(
        const QString& workspace_id,
        QList<app::client_gateway::WorkspacePresenceUser> users);
};

}  // namespace app::client_gateway
