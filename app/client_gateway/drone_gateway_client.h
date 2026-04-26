#pragma once

#include <QObject>

#include "app/client_gateway/client_gateway_types.h"
#include "app/mission/mission_workspace.h"

namespace app::client_gateway {

class IDroneGatewayClient : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;
    ~IDroneGatewayClient() override = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    [[nodiscard]] virtual GatewayConnectionState ConnectionState() const = 0;
    [[nodiscard]] virtual QList<DroneAgentEndpoint> DroneAgents() const = 0;
    [[nodiscard]] virtual GatewayResult ValidateDroneAllocation(
        const mission::MissionWorkspace& workspace) const = 0;

   signals:
    void SigConnectionChanged(app::client_gateway::GatewayConnectionState state);
    void SigDroneAgentsChanged(QList<app::client_gateway::DroneAgentEndpoint> agents);
};

}  // namespace app::client_gateway
