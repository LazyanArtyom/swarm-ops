#include "app/client_gateway/simulated_client_gateway.h"

namespace app::client_gateway {

SimulatedClientGateway::SimulatedClientGateway(QObject* parent)
    : IClientGateway(parent), mission_workspaces_(this), drone_gateway_(this), swarm_runtime_(this) {
    mission_workspaces_.Start();
    drone_gateway_.Start();
    swarm_runtime_.Start();
}

IMissionWorkspaceClient& SimulatedClientGateway::MissionWorkspaces() {
    return mission_workspaces_;
}

IDroneGatewayClient& SimulatedClientGateway::DroneGateway() {
    return drone_gateway_;
}

ISwarmRuntimeClient& SimulatedClientGateway::SwarmRuntime() {
    return swarm_runtime_;
}

}  // namespace app::client_gateway
