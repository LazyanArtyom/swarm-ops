#pragma once

#include <QObject>
#include <QString>

#include "app/client_gateway/client_gateway_types.h"

namespace app::client_gateway {

class ISwarmRuntimeClient : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;
    ~ISwarmRuntimeClient() override = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    [[nodiscard]] virtual GatewayConnectionState ConnectionState() const = 0;
    [[nodiscard]] virtual SwarmStateSnapshot LatestSwarmState(const QString& swarm_id) const = 0;
    [[nodiscard]] virtual CommandReceipt SendCommand(SwarmCommand command) = 0;
    [[nodiscard]] virtual GatewayResult StartMissionSimulation(
        const mission::MissionWorkspace& workspace) = 0;
    virtual void PauseMissionSimulation() = 0;
    virtual void ResumeMissionSimulation() = 0;
    virtual void StopMissionSimulation() = 0;
    [[nodiscard]] virtual MissionSimulationFrame LatestMissionSimulationFrame() const = 0;

   signals:
    void SigConnectionChanged(app::client_gateway::GatewayConnectionState state);
    void SigSwarmStateUpdated(const app::client_gateway::SwarmStateSnapshot& snapshot);
    void SigCommandAcknowledged(const app::client_gateway::CommandReceipt& receipt);
    void SigAlertReceived(const app::client_gateway::AlertEvent& alert);
    void SigMissionSimulationFrame(const app::client_gateway::MissionSimulationFrame& frame);
};

}  // namespace app::client_gateway
