#pragma once

#include <QPointer>

#include "app/mission/mission_workspace_gateway.h"

namespace app::client_gateway {
class IMissionWorkspaceClient;
}  // namespace app::client_gateway

namespace app::mission {

class ClientMissionWorkspaceGateway final : public IMissionWorkspaceGateway {
    Q_OBJECT

   public:
    explicit ClientMissionWorkspaceGateway(client_gateway::IMissionWorkspaceClient* client,
                                           QObject* parent = nullptr);

    [[nodiscard]] MissionWorkspace ActiveWorkspace() const override;
    [[nodiscard]] QString CreateWorkspace(const WorkspaceBackground& background) override;
    void ReplaceWorkspace(MissionWorkspace workspace) override;

   private:
    QPointer<client_gateway::IMissionWorkspaceClient> client_;
    MissionWorkspace active_workspace_;
};

}  // namespace app::mission
