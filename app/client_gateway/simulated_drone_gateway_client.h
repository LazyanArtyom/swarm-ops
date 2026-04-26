#pragma once

#include "app/client_gateway/drone_gateway_client.h"

namespace app::client_gateway {

class SimulatedDroneGatewayClient final : public IDroneGatewayClient {
    Q_OBJECT

   public:
    explicit SimulatedDroneGatewayClient(QObject* parent = nullptr);

    void Start() override;
    void Stop() override;
    [[nodiscard]] GatewayConnectionState ConnectionState() const override;
    [[nodiscard]] QList<DroneAgentEndpoint> DroneAgents() const override;
    [[nodiscard]] GatewayResult ValidateDroneAllocation(
        const mission::MissionWorkspace& workspace) const override;

   private:
    QList<DroneAgentEndpoint> agents_;
    GatewayConnectionState state_{GatewayConnectionState::kDisconnected};
};

}  // namespace app::client_gateway
