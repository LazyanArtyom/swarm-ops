#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QTimer>

#include "app/client_gateway/swarm_runtime_client.h"

namespace app::client_gateway {

class SimulatedSwarmRuntimeClient final : public ISwarmRuntimeClient {
    Q_OBJECT

   public:
    explicit SimulatedSwarmRuntimeClient(QObject* parent = nullptr);

    void Start() override;
    void Stop() override;
    [[nodiscard]] GatewayConnectionState ConnectionState() const override;
    [[nodiscard]] SwarmStateSnapshot LatestSwarmState(const QString& swarm_id) const override;
    [[nodiscard]] CommandReceipt SendCommand(SwarmCommand command) override;
    [[nodiscard]] GatewayResult StartMissionSimulation(
        const mission::MissionWorkspace& workspace) override;
    void PauseMissionSimulation() override;
    void ResumeMissionSimulation() override;
    void StopMissionSimulation() override;
    [[nodiscard]] MissionSimulationFrame LatestMissionSimulationFrame() const override;

   private:
    struct SimulationNode final {
        QString id;
        QPointF position;
        mission::GraphNodeType type{mission::GraphNodeType::kGeneric};
        mission::GraphNodeCategory category{mission::GraphNodeCategory::kGeneric};
        QList<int> neighbours;
        int current_neighbour_index{-1};
    };

    void Tick();
    void TickMissionSimulation();
    [[nodiscard]] GatewayResult PrepareMissionSimulation(const mission::MissionWorkspace& workspace);
    void EmitSimulationFrame(QString message = {});
    [[nodiscard]] bool MissionSimulationCompleted() const;
    [[nodiscard]] bool IsDroneAllowedToFinish(int drone_index);
    [[nodiscard]] int CurrentNeighbourIndex(int node_index) const;
    [[nodiscard]] QString EdgeKey(int from_index, int to_index) const;
    void IncrementCurrentNeighbour(int node_index);
    void BeginDroneEdge(int drone_index);
    void AppendTrailSegment(int drone_index, QPointF start_position, QPointF end_position);
    [[nodiscard]] SwarmStateSnapshot MakeSnapshot(double elapsed_seconds) const;

    QTimer tick_timer_;
    QTimer simulation_timer_;
    QElapsedTimer elapsed_;
    GatewayConnectionState state_{GatewayConnectionState::kDisconnected};
    SwarmStateSnapshot latest_;
    MissionSimulationFrame latest_simulation_frame_;
    QList<SimulationNode> simulation_nodes_;
    QHash<QString, int> simulation_node_index_;
    QList<int> drone_start_nodes_;
    QList<int> drone_current_nodes_;
    QList<int> drone_possible_nodes_;
    QList<int> drone_edge_pass_counts_;
    QList<int> drone_start_directions_;
    QList<bool> drone_landed_at_start_;
    QList<bool> drone_landed_;
    QList<QPointF> drone_positions_;
    QList<SimulatedDroneTrailSegment> pending_trail_segments_;
    QString simulation_workspace_id_;
    QHash<QString, int> edge_visit_counts_;
    MissionSimulationState simulation_state_{MissionSimulationState::kIdle};
    int simulation_frame_index_{0};
    double simulation_step_distance_px_{25.0};
};

}  // namespace app::client_gateway
