#include "app/client_gateway/api_gateway_client.h"

#include <QDateTime>

namespace app::client_gateway {
namespace {

GatewayResult NotImplemented() {
    return GatewayResult::Failure(QObject::tr("API Gateway transport is not configured yet."));
}

}  // namespace

void ApiGatewayMissionWorkspaceClient::Start() {
    emit SigConnectionChanged(GatewayConnectionState::kFaulted);
}

void ApiGatewayMissionWorkspaceClient::Stop() {
    emit SigConnectionChanged(GatewayConnectionState::kDisconnected);
}

GatewayConnectionState ApiGatewayMissionWorkspaceClient::ConnectionState() const {
    return GatewayConnectionState::kFaulted;
}

QList<WorkspaceListItem> ApiGatewayMissionWorkspaceClient::ListWorkspaces() const {
    return {};
}

mission::MissionWorkspace ApiGatewayMissionWorkspaceClient::ActiveWorkspace() const {
    return {};
}

mission::MissionWorkspace ApiGatewayMissionWorkspaceClient::GetWorkspaceSnapshot(
    const QString& /*workspace_id*/) const {
    return {};
}

QString ApiGatewayMissionWorkspaceClient::CreateWorkspace(
    QString /*name*/, const mission::WorkspaceBackground& /*background*/) {
    return {};
}

GatewayResult ApiGatewayMissionWorkspaceClient::SaveWorkspaceSnapshot(
    mission::MissionWorkspace /*workspace*/) {
    return NotImplemented();
}

GatewayResult ApiGatewayMissionWorkspaceClient::DeleteWorkspace(
    const QString& /*workspace_id*/) {
    return NotImplemented();
}

GatewayResult ApiGatewayMissionWorkspaceClient::JoinWorkspaceSession(
    const QString& /*workspace_id*/, QString /*user_display_name*/) {
    return NotImplemented();
}

void ApiGatewayMissionWorkspaceClient::LeaveWorkspaceSession() {}

GatewayResult ApiGatewayMissionWorkspaceClient::ApplyWorkspaceSnapshot(
    mission::MissionWorkspace /*workspace*/, WorkspaceUpdateType /*update_type*/) {
    return NotImplemented();
}

WorkspaceCheckpoint ApiGatewayMissionWorkspaceClient::CreateCheckpoint(
    const QString& /*workspace_id*/, QString /*name*/) {
    return {};
}

void ApiGatewayDroneGatewayClient::Start() {
    emit SigConnectionChanged(GatewayConnectionState::kFaulted);
}

void ApiGatewayDroneGatewayClient::Stop() {
    emit SigConnectionChanged(GatewayConnectionState::kDisconnected);
}

GatewayConnectionState ApiGatewayDroneGatewayClient::ConnectionState() const {
    return GatewayConnectionState::kFaulted;
}

QList<DroneAgentEndpoint> ApiGatewayDroneGatewayClient::DroneAgents() const {
    return {};
}

GatewayResult ApiGatewayDroneGatewayClient::ValidateDroneAllocation(
    const mission::MissionWorkspace& /*workspace*/) const {
    return NotImplemented();
}

void ApiGatewaySwarmRuntimeClient::Start() {
    emit SigConnectionChanged(GatewayConnectionState::kFaulted);
}

void ApiGatewaySwarmRuntimeClient::Stop() {
    emit SigConnectionChanged(GatewayConnectionState::kDisconnected);
}

GatewayConnectionState ApiGatewaySwarmRuntimeClient::ConnectionState() const {
    return GatewayConnectionState::kFaulted;
}

SwarmStateSnapshot ApiGatewaySwarmRuntimeClient::LatestSwarmState(
    const QString& /*swarm_id*/) const {
    return {};
}

CommandReceipt ApiGatewaySwarmRuntimeClient::SendCommand(SwarmCommand command) {
    CommandReceipt receipt;
    receipt.command_id = command.command_id;
    receipt.accepted = false;
    receipt.accepted_at = QDateTime::currentDateTimeUtc();
    receipt.message = tr("API Gateway transport is not configured yet.");
    emit SigCommandAcknowledged(receipt);
    return receipt;
}

GatewayResult ApiGatewaySwarmRuntimeClient::StartMissionSimulation(
    const mission::MissionWorkspace& /*workspace*/) {
    return NotImplemented();
}

void ApiGatewaySwarmRuntimeClient::PauseMissionSimulation() {}

void ApiGatewaySwarmRuntimeClient::ResumeMissionSimulation() {}

void ApiGatewaySwarmRuntimeClient::StopMissionSimulation() {}

MissionSimulationFrame ApiGatewaySwarmRuntimeClient::LatestMissionSimulationFrame() const {
    return {};
}

ApiGatewayClient::ApiGatewayClient(QObject* parent)
    : IClientGateway(parent), mission_workspaces_(this), drone_gateway_(this), swarm_runtime_(this) {}

IMissionWorkspaceClient& ApiGatewayClient::MissionWorkspaces() {
    return mission_workspaces_;
}

IDroneGatewayClient& ApiGatewayClient::DroneGateway() {
    return drone_gateway_;
}

ISwarmRuntimeClient& ApiGatewayClient::SwarmRuntime() {
    return swarm_runtime_;
}

}  // namespace app::client_gateway
