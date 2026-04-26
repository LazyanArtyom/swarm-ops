#include "app/client_gateway/simulated_swarm_runtime_client.h"

#include <QDateTime>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <utility>

namespace app::client_gateway {
namespace {

constexpr int kTelemetryTickMs = 500;
constexpr double kBaseLatitudeDeg = 40.1792;
constexpr double kBaseLongitudeDeg = 44.4991;
constexpr double kPi = 3.14159265358979323846;
constexpr int kMissionSimulationTickMs = 16;
constexpr double kSimulationStepDistancePx = 25.0;
constexpr double kMinSimulationSpeedMultiplier = 0.10;
constexpr double kMaxSimulationSpeedMultiplier = 2.0;
constexpr double kPointDistanceEpsilon = 0.000001;
constexpr double kTrailSegmentMinLengthSquared = 0.01;
constexpr int kSimulationGraphGuardMultiplier = 4;
constexpr int kSimulatedTelemetryDroneCount = 6;

QString NewId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString SimulatedDroneId(int index) {
    return QStringLiteral("sim-drone-%1").arg(index + 1);
}

double DistanceBetween(QPointF lhs, QPointF rhs) {
    const QPointF delta = lhs - rhs;
    return std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
}

QPointF PointOnSegment(QPointF from, QPointF to, double distance) {
    const QPointF delta = to - from;
    const double length = DistanceBetween(from, to);
    if (length <= kPointDistanceEpsilon) {
        return to;
    }
    const QPointF unit(delta.x() / length, delta.y() / length);
    return QPointF(from.x() + distance * unit.x(), from.y() + distance * unit.y());
}

bool IsMeaningfulSegment(QPointF from, QPointF to) {
    const QPointF delta = to - from;
    return (delta.x() * delta.x() + delta.y() * delta.y()) > kTrailSegmentMinLengthSquared;
}

double AngleFrom(QPointF origin, QPointF point) {
    const QPointF delta = point - origin;
    return std::atan2(delta.y(), delta.x());
}

double WrapHeading(double degrees) {
    const double wrapped = std::fmod(degrees, 360.0);
    return wrapped < 0.0 ? wrapped + 360.0 : wrapped;
}

}  // namespace

SimulatedSwarmRuntimeClient::SimulatedSwarmRuntimeClient(QObject* parent)
    : ISwarmRuntimeClient(parent) {
    tick_timer_.setInterval(kTelemetryTickMs);
    connect(&tick_timer_, &QTimer::timeout, this, &SimulatedSwarmRuntimeClient::Tick);
    simulation_timer_.setInterval(kMissionSimulationTickMs);
    connect(&simulation_timer_, &QTimer::timeout, this,
            &SimulatedSwarmRuntimeClient::TickMissionSimulation);
}

void SimulatedSwarmRuntimeClient::Start() {
    if (state_ == GatewayConnectionState::kConnected) {
        return;
    }

    state_ = GatewayConnectionState::kConnecting;
    emit SigConnectionChanged(state_);
    elapsed_.restart();
    state_ = GatewayConnectionState::kConnected;
    emit SigConnectionChanged(state_);
    tick_timer_.start();
    Tick();
}

void SimulatedSwarmRuntimeClient::Stop() {
    if (state_ == GatewayConnectionState::kDisconnected) {
        return;
    }

    tick_timer_.stop();
    simulation_timer_.stop();
    simulation_state_ = MissionSimulationState::kIdle;
    run_mode_ = RunMode::kNone;
    state_ = GatewayConnectionState::kDisconnected;
    emit SigConnectionChanged(state_);
}

GatewayConnectionState SimulatedSwarmRuntimeClient::ConnectionState() const {
    return state_;
}

SwarmStateSnapshot SimulatedSwarmRuntimeClient::LatestSwarmState(const QString& swarm_id) const {
    if (swarm_id.isEmpty() || latest_.swarm_id == swarm_id) {
        return latest_;
    }
    return {};
}

CommandReceipt SimulatedSwarmRuntimeClient::SendCommand(SwarmCommand command) {
    if (command.command_id.isEmpty()) {
        command.command_id = NewId();
    }

    CommandReceipt receipt;
    receipt.command_id = command.command_id;
    receipt.accepted = state_ == GatewayConnectionState::kConnected;
    receipt.accepted_at = QDateTime::currentDateTimeUtc();
    receipt.message = receipt.accepted ? tr("Simulator accepted command.")
                                       : tr("Simulator is disconnected.");
    emit SigCommandAcknowledged(receipt);
    return receipt;
}

GatewayResult SimulatedSwarmRuntimeClient::StartMissionSimulation(
    const mission::MissionWorkspace& workspace) {
    const GatewayResult prepare_result = PrepareMissionSimulation(workspace);
    if (!prepare_result.ok) {
        run_mode_ = RunMode::kSimulation;
        latest_simulation_frame_ = {
            .workspace_id = workspace.id,
            .state = MissionSimulationState::kRejected,
            .message = prepare_result.message,
            .frame_index = simulation_frame_index_,
        };
        emit SigMissionSimulationFrame(latest_simulation_frame_);
        return prepare_result;
    }

    run_mode_ = RunMode::kSimulation;
    simulation_state_ = MissionSimulationState::kRunning;
    simulation_timer_.start();
    EmitSimulationFrame(tr("Mission simulation started."));
    return GatewayResult::Success(tr("Mission simulation started."));
}

void SimulatedSwarmRuntimeClient::PauseMissionSimulation() {
    if (run_mode_ != RunMode::kSimulation ||
        simulation_state_ != MissionSimulationState::kRunning) {
        return;
    }

    simulation_timer_.stop();
    simulation_state_ = MissionSimulationState::kPaused;
    EmitSimulationFrame(tr("Mission simulation paused."));
}

void SimulatedSwarmRuntimeClient::ResumeMissionSimulation() {
    if (run_mode_ != RunMode::kSimulation ||
        simulation_state_ != MissionSimulationState::kPaused) {
        return;
    }

    simulation_state_ = MissionSimulationState::kRunning;
    simulation_timer_.start();
    EmitSimulationFrame(tr("Mission simulation resumed."));
}

void SimulatedSwarmRuntimeClient::StopMissionSimulation() {
    if (run_mode_ != RunMode::kSimulation || simulation_state_ == MissionSimulationState::kIdle) {
        return;
    }

    simulation_timer_.stop();
    simulation_state_ = MissionSimulationState::kIdle;
    EmitSimulationFrame(tr("Mission simulation stopped."));
    run_mode_ = RunMode::kNone;
}

void SimulatedSwarmRuntimeClient::SetMissionSimulationSpeedMultiplier(double multiplier) {
    simulation_speed_multiplier_ =
        std::clamp(multiplier, kMinSimulationSpeedMultiplier, kMaxSimulationSpeedMultiplier);
}

double SimulatedSwarmRuntimeClient::MissionSimulationSpeedMultiplier() const {
    return simulation_speed_multiplier_;
}

MissionSimulationFrame SimulatedSwarmRuntimeClient::LatestMissionSimulationFrame() const {
    return latest_simulation_frame_;
}

GatewayResult SimulatedSwarmRuntimeClient::StartLiveMission(
    const mission::MissionWorkspace& workspace) {
    const GatewayResult prepare_result = PrepareMissionSimulation(workspace);
    if (!prepare_result.ok) {
        run_mode_ = RunMode::kLiveMission;
        latest_live_mission_frame_ = {
            .workspace_id = workspace.id,
            .state = MissionSimulationState::kRejected,
            .message = prepare_result.message,
            .frame_index = simulation_frame_index_,
        };
        emit SigLiveMissionFrame(latest_live_mission_frame_);
        return prepare_result;
    }

    run_mode_ = RunMode::kLiveMission;
    simulation_state_ = MissionSimulationState::kRunning;
    simulation_timer_.start();
    EmitSimulationFrame(tr("Live mission telemetry started."));
    return GatewayResult::Success(tr("Live mission started."));
}

void SimulatedSwarmRuntimeClient::StopLiveMission() {
    if (run_mode_ != RunMode::kLiveMission || simulation_state_ == MissionSimulationState::kIdle) {
        return;
    }

    simulation_timer_.stop();
    simulation_state_ = MissionSimulationState::kIdle;
    EmitSimulationFrame(tr("Live mission stopped."));
    run_mode_ = RunMode::kNone;
}

MissionSimulationFrame SimulatedSwarmRuntimeClient::LatestLiveMissionFrame() const {
    return latest_live_mission_frame_;
}

void SimulatedSwarmRuntimeClient::Tick() {
    if (state_ != GatewayConnectionState::kConnected) {
        return;
    }

    latest_ = MakeSnapshot(static_cast<double>(elapsed_.elapsed()) / 1000.0);
    emit SigSwarmStateUpdated(latest_);
}

void SimulatedSwarmRuntimeClient::TickMissionSimulation() {
    if (simulation_state_ != MissionSimulationState::kRunning) {
        return;
    }

    for (qsizetype drone_index = 0; drone_index < drone_current_nodes_.size(); ++drone_index) {
        double distance_remaining = kSimulationStepDistancePx * simulation_speed_multiplier_;
        if (drone_landed_.value(drone_index)) {
            continue;
        }

        while (distance_remaining > 0.0) {
            const int possible_index = drone_possible_nodes_.value(drone_index, -1);
            if (possible_index < 0 || possible_index >= simulation_nodes_.size()) {
                drone_landed_[drone_index] = true;
                break;
            }

            const QPointF possible_position = simulation_nodes_[possible_index].position;
            const double current_distance =
                DistanceBetween(drone_positions_[drone_index], possible_position);
            if (distance_remaining >= current_distance) {
                const QPointF segment_start = drone_positions_[drone_index];
                distance_remaining -= current_distance;
                drone_positions_[drone_index] = possible_position;
                AppendTrailSegment(static_cast<int>(drone_index), segment_start, possible_position);
                if (IsDroneAllowedToFinish(static_cast<int>(drone_index))) {
                    drone_landed_[drone_index] = true;
                    break;
                }
                drone_current_nodes_[drone_index] = possible_index;
                IncrementCurrentNeighbour(possible_index);
                drone_possible_nodes_[drone_index] = CurrentNeighbourIndex(possible_index);
                BeginDroneEdge(static_cast<int>(drone_index));
            } else {
                const QPointF segment_start = drone_positions_[drone_index];
                const QPointF segment_end =
                    PointOnSegment(segment_start, possible_position, distance_remaining);
                drone_positions_[drone_index] = segment_end;
                AppendTrailSegment(static_cast<int>(drone_index), segment_start, segment_end);
                break;
            }
        }
    }

    if (MissionSimulationCompleted()) {
        simulation_state_ = MissionSimulationState::kCompleted;
        simulation_timer_.stop();
        EmitSimulationFrame(run_mode_ == RunMode::kLiveMission ? tr("Live mission completed.")
                                                                : tr("Mission simulation completed."));
        run_mode_ = RunMode::kNone;
        return;
    }

    EmitSimulationFrame();
}

GatewayResult SimulatedSwarmRuntimeClient::PrepareMissionSimulation(
    const mission::MissionWorkspace& workspace) {
    simulation_timer_.stop();
    simulation_workspace_id_ = workspace.id;
    simulation_state_ = MissionSimulationState::kIdle;
    run_mode_ = RunMode::kNone;
    simulation_frame_index_ = 0;
    latest_simulation_frame_ = {};
    latest_live_mission_frame_ = {};
    simulation_nodes_.clear();
    simulation_node_index_.clear();
    drone_start_nodes_.clear();
    drone_current_nodes_.clear();
    drone_possible_nodes_.clear();
    drone_edge_pass_counts_.clear();
    drone_start_directions_.clear();
    drone_landed_at_start_.clear();
    drone_landed_.clear();
    drone_positions_.clear();
    pending_trail_segments_.clear();
    edge_visit_counts_.clear();

    if (workspace.id.isEmpty() || !workspace.background.IsValid()) {
        return GatewayResult::Failure(tr("Open a mission workspace with a background image first."));
    }

    simulation_nodes_.reserve(workspace.nodes.size());
    for (const mission::GraphNode& node : workspace.nodes) {
        const int index = simulation_nodes_.size();
        simulation_node_index_.insert(node.id, index);
        simulation_nodes_.push_back({
            .id = node.id,
            .position = node.position,
            .type = node.type,
            .category = node.category,
        });
    }

    for (const mission::GraphEdge& edge : workspace.edges) {
        const int from_index = simulation_node_index_.value(edge.from_node_id, -1);
        const int to_index = simulation_node_index_.value(edge.to_node_id, -1);
        if (from_index < 0 || to_index < 0 || from_index == to_index) {
            continue;
        }
        simulation_nodes_[from_index].neighbours.push_back(to_index);
        simulation_nodes_[to_index].neighbours.push_back(from_index);
    }

    for (SimulationNode& node : simulation_nodes_) {
        const QPointF origin = node.position;
        std::sort(node.neighbours.begin(), node.neighbours.end(),
                  [this, origin](int lhs, int rhs) {
                      return AngleFrom(origin, simulation_nodes_[lhs].position) <
                             AngleFrom(origin, simulation_nodes_[rhs].position);
                  });
    }

    int start_node_index = -1;
    qsizetype neighbours_count = 0;
    for (int index = 0; index < simulation_nodes_.size(); ++index) {
        neighbours_count += simulation_nodes_[index].neighbours.size();
        if (simulation_nodes_[index].category == mission::GraphNodeCategory::kDrone) {
            drone_start_nodes_.push_back(index);
        }
        if (start_node_index < 0 &&
            (simulation_nodes_[index].type == mission::GraphNodeType::kBorder ||
             simulation_nodes_[index].type == mission::GraphNodeType::kCorner)) {
            start_node_index = index;
        }
    }

    if (simulation_nodes_.isEmpty() || drone_start_nodes_.isEmpty()) {
        return GatewayResult::Failure(tr("Set at least one graph node as a drone."));
    }
    if (start_node_index < 0 || simulation_nodes_[start_node_index].neighbours.isEmpty()) {
        return GatewayResult::Failure(tr("The mission graph needs at least one connected border or corner node."));
    }

    auto is_border_like = [this](int index) {
        return index >= 0 && index < simulation_nodes_.size() &&
               (simulation_nodes_[index].type == mission::GraphNodeType::kBorder ||
                simulation_nodes_[index].type == mission::GraphNodeType::kCorner);
    };

    for (int i = 0; i < simulation_nodes_[start_node_index].neighbours.size(); ++i) {
        if (is_border_like(simulation_nodes_[start_node_index].neighbours[i])) {
            simulation_nodes_[start_node_index].current_neighbour_index = i;
            break;
        }
    }
    if (simulation_nodes_[start_node_index].current_neighbour_index < 0) {
        simulation_nodes_[start_node_index].current_neighbour_index = 0;
    }

    int node_index = CurrentNeighbourIndex(start_node_index);
    int guard = 0;
    while (node_index != start_node_index &&
           guard++ < simulation_nodes_.size() * kSimulationGraphGuardMultiplier) {
        SimulationNode& node = simulation_nodes_[node_index];
        for (int i = 0; i < node.neighbours.size(); ++i) {
            const int neighbour_index = node.neighbours[i];
            if (is_border_like(neighbour_index) &&
                simulation_nodes_[neighbour_index].current_neighbour_index == -1) {
                node.current_neighbour_index = i;
                break;
            }
        }
        if (node.current_neighbour_index == -1) {
            for (int i = 0; i < node.neighbours.size(); ++i) {
                if (node.neighbours[i] == start_node_index) {
                    node.current_neighbour_index = i;
                    break;
                }
            }
        }
        if (node.current_neighbour_index == -1 && !node.neighbours.isEmpty()) {
            node.current_neighbour_index = 0;
        }
        node_index = CurrentNeighbourIndex(node_index);
        if (node_index < 0) {
            return GatewayResult::Failure(tr("The mission graph is not connected enough for simulation."));
        }
    }

    for (SimulationNode& node : simulation_nodes_) {
        if (node.current_neighbour_index == -1 && !node.neighbours.isEmpty()) {
            node.current_neighbour_index =
                static_cast<int>(qHash(node.id) % static_cast<uint>(node.neighbours.size()));
        }
    }

    int current_index = start_node_index;
    const int start_direction = simulation_nodes_[start_node_index].current_neighbour_index;
    QSet<QPair<int, int>> unique_steps;
    guard = 0;
    while (guard++ < std::max<qsizetype>(1, neighbours_count * kSimulationGraphGuardMultiplier)) {
        IncrementCurrentNeighbour(current_index);
        const int next_index = CurrentNeighbourIndex(current_index);
        if (next_index < 0) {
            break;
        }
        unique_steps.insert(qMakePair(current_index, next_index));
        current_index = next_index;
        if (current_index == start_node_index &&
            simulation_nodes_[current_index].current_neighbour_index == start_direction &&
            unique_steps.size() >= neighbours_count) {
            break;
        }
    }

    for (int drone_node_index : drone_start_nodes_) {
        drone_current_nodes_.push_back(drone_node_index);
        drone_start_directions_.push_back(simulation_nodes_[drone_node_index].current_neighbour_index);
        IncrementCurrentNeighbour(drone_node_index);
        const int possible_node_index = CurrentNeighbourIndex(drone_node_index);
        if (possible_node_index < 0) {
            return GatewayResult::Failure(
                tr("Every drone node must be connected to at least one graph edge."));
        }
        drone_possible_nodes_.push_back(possible_node_index);
        drone_edge_pass_counts_.push_back(0);
        drone_landed_at_start_.push_back(false);
        drone_landed_.push_back(false);
        drone_positions_.push_back(simulation_nodes_[drone_node_index].position);
        BeginDroneEdge(drone_current_nodes_.size() - 1);
    }

    return GatewayResult::Success();
}

void SimulatedSwarmRuntimeClient::EmitSimulationFrame(QString message) {
    MissionSimulationFrame frame;
    frame.workspace_id = simulation_workspace_id_;
    frame.state = simulation_state_;
    frame.message = std::move(message);
    frame.frame_index = simulation_frame_index_++;
    frame.trail_segments = std::move(pending_trail_segments_);
    frame.drones.reserve(drone_positions_.size());
    for (qsizetype i = 0; i < drone_positions_.size(); ++i) {
        const int start_index = drone_start_nodes_.value(i, -1);
        const int from_index = drone_current_nodes_.value(i, -1);
        const int to_index = drone_possible_nodes_.value(i, -1);
        frame.drones.push_back({
            .node_id = start_index >= 0 && start_index < simulation_nodes_.size()
                           ? simulation_nodes_[start_index].id
                           : QString(),
            .drone_id = SimulatedDroneId(static_cast<int>(i)),
            .from_node_id = from_index >= 0 && from_index < simulation_nodes_.size()
                                ? simulation_nodes_[from_index].id
                                : QString(),
            .to_node_id = to_index >= 0 && to_index < simulation_nodes_.size()
                              ? simulation_nodes_[to_index].id
                              : QString(),
            .position = drone_positions_[i],
            .edge_pass_index = drone_edge_pass_counts_.value(i),
            .landed = drone_landed_.value(i),
        });
    }
    if (run_mode_ == RunMode::kLiveMission) {
        latest_live_mission_frame_ = frame;
        emit SigLiveMissionFrame(latest_live_mission_frame_);
        return;
    }

    latest_simulation_frame_ = frame;
    emit SigMissionSimulationFrame(latest_simulation_frame_);
}

bool SimulatedSwarmRuntimeClient::MissionSimulationCompleted() const {
    return std::all_of(drone_landed_at_start_.begin(), drone_landed_at_start_.end(),
                       [](bool landed) { return landed; });
}

bool SimulatedSwarmRuntimeClient::IsDroneAllowedToFinish(int drone_index) {
    const int possible_index = drone_possible_nodes_.value(drone_index, -1);
    for (int i = 0; i < drone_start_nodes_.size(); ++i) {
        if (possible_index == drone_start_nodes_[i] &&
            simulation_nodes_[possible_index].current_neighbour_index == drone_start_directions_[i] &&
            !drone_landed_at_start_[i]) {
            drone_landed_at_start_[i] = true;
            return true;
        }
    }
    return false;
}

int SimulatedSwarmRuntimeClient::CurrentNeighbourIndex(int node_index) const {
    if (node_index < 0 || node_index >= simulation_nodes_.size()) {
        return -1;
    }
    const SimulationNode& node = simulation_nodes_[node_index];
    if (node.neighbours.isEmpty() || node.current_neighbour_index < 0) {
        return -1;
    }
    return node.neighbours[node.current_neighbour_index % node.neighbours.size()];
}

QString SimulatedSwarmRuntimeClient::EdgeKey(int from_index, int to_index) const {
    if (from_index < 0 || to_index < 0 || from_index >= simulation_nodes_.size() ||
        to_index >= simulation_nodes_.size()) {
        return {};
    }
    const QString& lhs = simulation_nodes_[from_index].id;
    const QString& rhs = simulation_nodes_[to_index].id;
    return lhs < rhs ? QStringLiteral("%1:%2").arg(lhs, rhs)
                     : QStringLiteral("%1:%2").arg(rhs, lhs);
}

void SimulatedSwarmRuntimeClient::IncrementCurrentNeighbour(int node_index) {
    if (node_index < 0 || node_index >= simulation_nodes_.size() ||
        simulation_nodes_[node_index].neighbours.isEmpty()) {
        return;
    }
    SimulationNode& node = simulation_nodes_[node_index];
    node.current_neighbour_index =
        node.current_neighbour_index < 0 ? 0 : (node.current_neighbour_index + 1) % node.neighbours.size();
}

void SimulatedSwarmRuntimeClient::BeginDroneEdge(int drone_index) {
    if (drone_index < 0 || drone_index >= drone_current_nodes_.size()) {
        return;
    }
    const QString edge_key =
        EdgeKey(drone_current_nodes_.value(drone_index), drone_possible_nodes_.value(drone_index));
    if (edge_key.isEmpty()) {
        drone_edge_pass_counts_[drone_index] = 0;
        return;
    }
    const QString drone_edge_key = QStringLiteral("%1:%2").arg(drone_index).arg(edge_key);
    int pass_count = edge_visit_counts_.value(drone_edge_key, 0) + 1;
    edge_visit_counts_.insert(drone_edge_key, pass_count);
    drone_edge_pass_counts_[drone_index] = pass_count;
}

void SimulatedSwarmRuntimeClient::AppendTrailSegment(int drone_index, QPointF start_position,
                                                     QPointF end_position) {
    if (drone_index < 0 || drone_index >= drone_current_nodes_.size()) {
        return;
    }

    if (!IsMeaningfulSegment(start_position, end_position)) {
        return;
    }

    const int from_index = drone_current_nodes_.value(drone_index, -1);
    const int to_index = drone_possible_nodes_.value(drone_index, -1);
    pending_trail_segments_.push_back({
        .drone_id = SimulatedDroneId(drone_index),
        .from_node_id = from_index >= 0 && from_index < simulation_nodes_.size()
                            ? simulation_nodes_[from_index].id
                            : QString(),
        .to_node_id = to_index >= 0 && to_index < simulation_nodes_.size()
                          ? simulation_nodes_[to_index].id
                          : QString(),
        .start_position = start_position,
        .end_position = end_position,
        .edge_pass_index = drone_edge_pass_counts_.value(drone_index),
    });
}

SwarmStateSnapshot SimulatedSwarmRuntimeClient::MakeSnapshot(double elapsed_seconds) const {
    SwarmStateSnapshot snapshot;
    snapshot.swarm_id = QStringLiteral("sim-swarm-1");
    snapshot.timestamp = QDateTime::currentDateTimeUtc();
    snapshot.drones.reserve(kSimulatedTelemetryDroneCount);

    for (int i = 0; i < kSimulatedTelemetryDroneCount; ++i) {
        const double lane = static_cast<double>(i);
        const double phase = elapsed_seconds * 0.18 + lane * (kPi / 3.0);
        const double ring_radius = 0.0022 + lane * 0.00008;

        DroneTelemetry telemetry;
        telemetry.drone_id = SimulatedDroneId(i);
        telemetry.callsign = QStringLiteral("D%1").arg(i + 1);
        telemetry.latitude_deg = kBaseLatitudeDeg + std::sin(phase) * ring_radius;
        telemetry.longitude_deg = kBaseLongitudeDeg + std::cos(phase) * ring_radius;
        telemetry.altitude_m = 70.0 + std::sin(phase * 1.7) * 6.0 + lane;
        telemetry.heading_deg = WrapHeading(phase * 180.0 / kPi + 90.0);
        telemetry.velocity_mps = 8.0 + std::cos(phase) * 1.5;
        telemetry.battery_percent = std::max(18.0, 96.0 - elapsed_seconds * 0.015 - lane * 2.2);
        telemetry.health = telemetry.battery_percent < 25.0 ? DroneHealth::kWarning
                                                            : DroneHealth::kNominal;
        telemetry.timestamp = snapshot.timestamp;
        snapshot.drones.push_back(telemetry);
    }

    return snapshot;
}

}  // namespace app::client_gateway
