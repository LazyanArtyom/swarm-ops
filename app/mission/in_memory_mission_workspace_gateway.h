#pragma once

#include <QHash>

#include "app/mission/mission_workspace_gateway.h"

namespace app::mission {

class InMemoryMissionWorkspaceGateway final : public IMissionWorkspaceGateway {
    Q_OBJECT

   public:
    explicit InMemoryMissionWorkspaceGateway(QObject* parent = nullptr);

    [[nodiscard]] MissionWorkspace ActiveWorkspace() const override;
    [[nodiscard]] QList<client_gateway::WorkspaceListItem> ListWorkspaces() const override;
    [[nodiscard]] QString CreateWorkspace(const WorkspaceBackground& background) override;
    [[nodiscard]] bool OpenWorkspace(const QString& workspace_id, QString user_display_name,
                                     QString* error_message = nullptr) override;
    [[nodiscard]] bool SaveWorkspace(MissionWorkspace workspace,
                                     QString* error_message = nullptr) override;
    [[nodiscard]] bool SetWorkspaceShared(const QString& workspace_id, bool shared,
                                          QString* error_message = nullptr) override;
    [[nodiscard]] client_gateway::WorkspaceShareInvite CreateShareInvite(
        const QString& workspace_id, QString* error_message = nullptr) override;
    void ReplaceWorkspace(MissionWorkspace workspace) override;

   private:
    QHash<QString, MissionWorkspace> workspaces_;
    QString active_workspace_id_;
    int next_workspace_number_{1};
};

}  // namespace app::mission
