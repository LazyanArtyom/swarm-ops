#include "app/client_gateway/simulated_drone_gateway_client.h"

namespace app::client_gateway {

SimulatedDroneGatewayClient::SimulatedDroneGatewayClient(QObject* parent)
    : IDroneGatewayClient(parent) {
    for (int i = 0; i < 5; ++i) {
        agents_.push_back({
            .agent_id = QStringLiteral("sim-agent-%1").arg(i + 1),
            .display_name = QStringLiteral("Simulator Drone %1").arg(i + 1),
            .ip_address = QStringLiteral("127.0.10.%1").arg(i + 11),
            .available = true,
        });
    }
}

void SimulatedDroneGatewayClient::Start() {
    if (state_ == GatewayConnectionState::kConnected) {
        return;
    }
    state_ = GatewayConnectionState::kConnecting;
    emit SigConnectionChanged(state_);
    state_ = GatewayConnectionState::kConnected;
    emit SigConnectionChanged(state_);
    emit SigDroneAgentsChanged(agents_);
}

void SimulatedDroneGatewayClient::Stop() {
    if (state_ == GatewayConnectionState::kDisconnected) {
        return;
    }
    state_ = GatewayConnectionState::kDisconnected;
    emit SigConnectionChanged(state_);
}

GatewayConnectionState SimulatedDroneGatewayClient::ConnectionState() const {
    return state_;
}

QList<DroneAgentEndpoint> SimulatedDroneGatewayClient::DroneAgents() const {
    return agents_;
}

GatewayResult SimulatedDroneGatewayClient::ValidateDroneAllocation(
    const mission::MissionWorkspace& workspace) const {
    int requested_drones = 0;
    for (const mission::GraphNode& node : workspace.nodes) {
        if (node.category == mission::GraphNodeCategory::kDrone) {
            ++requested_drones;
        }
    }

    int available_drones = 0;
    for (const DroneAgentEndpoint& agent : agents_) {
        if (agent.available) {
            ++available_drones;
        }
    }

    if (requested_drones > available_drones) {
        return GatewayResult::Failure(
            tr("Only %1 drone agents are available in the simulator.").arg(available_drones));
    }
    return GatewayResult::Success();
}

}  // namespace app::client_gateway
