#pragma once

#include <QHash>

#include "app/client_gateway/mission_workspace_client.h"

namespace app::client_gateway {

class SimulatedMissionWorkspaceClient final : public IMissionWorkspaceClient {
    Q_OBJECT

   public:
    explicit SimulatedMissionWorkspaceClient(QObject* parent = nullptr);

    void Start() override;
    void Stop() override;
    [[nodiscard]] GatewayConnectionState ConnectionState() const override;

    [[nodiscard]] QList<WorkspaceListItem> ListWorkspaces() const override;
    [[nodiscard]] mission::MissionWorkspace ActiveWorkspace() const override;
    [[nodiscard]] mission::MissionWorkspace GetWorkspaceSnapshot(
        const QString& workspace_id) const override;

    [[nodiscard]] QString CreateWorkspace(QString name,
                                          const mission::WorkspaceBackground& background) override;
    [[nodiscard]] GatewayResult SaveWorkspaceSnapshot(mission::MissionWorkspace workspace) override;
    [[nodiscard]] GatewayResult DeleteWorkspace(const QString& workspace_id) override;

    [[nodiscard]] GatewayResult JoinWorkspaceSession(const QString& workspace_id,
                                                     QString user_display_name) override;
    void LeaveWorkspaceSession() override;
    [[nodiscard]] GatewayResult ApplyWorkspaceSnapshot(
        mission::MissionWorkspace workspace, WorkspaceUpdateType update_type) override;

    [[nodiscard]] WorkspaceCheckpoint CreateCheckpoint(const QString& workspace_id,
                                                       QString name) override;

   private:
    struct StoredWorkspace final {
        mission::MissionWorkspace workspace;
        QString owner_display_name;
        QDateTime created_at;
        QDateTime updated_at;
        int revision{0};
        bool shared{true};
    };

    [[nodiscard]] WorkspaceListItem ToListItem(const StoredWorkspace& stored) const;
    [[nodiscard]] QList<WorkspacePresenceUser> PresenceFor(const QString& workspace_id) const;
    void EmitWorkspaceList();
    void EmitPresence(const QString& workspace_id);
    void EnsureConnected();

    QHash<QString, StoredWorkspace> workspaces_;
    QHash<QString, QList<WorkspaceCheckpoint>> checkpoints_;
    QHash<QString, QList<WorkspacePresenceUser>> presence_;
    QString active_workspace_id_;
    GatewayConnectionState state_{GatewayConnectionState::kDisconnected};
    int next_workspace_number_{1};
    int next_revision_{1};
};

}  // namespace app::client_gateway
