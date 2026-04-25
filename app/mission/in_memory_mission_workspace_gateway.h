#pragma once

#include <QHash>

#include "app/mission/mission_workspace_gateway.h"

namespace app::mission {

class InMemoryMissionWorkspaceGateway final : public IMissionWorkspaceGateway {
    Q_OBJECT

   public:
    explicit InMemoryMissionWorkspaceGateway(QObject* parent = nullptr);

    [[nodiscard]] MissionWorkspace ActiveWorkspace() const override;
    [[nodiscard]] QString CreateWorkspace(const WorkspaceBackground& background) override;
    void ReplaceWorkspace(MissionWorkspace workspace) override;

   private:
    QHash<QString, MissionWorkspace> workspaces_;
    QString active_workspace_id_;
    int next_workspace_number_{1};
};

}  // namespace app::mission
