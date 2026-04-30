#pragma once

#include <QDateTime>
#include <QList>
#include <QPointF>
#include <QVariantMap>
#include <QString>
#include <utility>

#include "app/mission/mission_workspace.h"

namespace app::client_gateway {

enum class GatewayConnectionState {
    kDisconnected,
    kConnecting,
    kConnected,
    kFaulted,
};

struct GatewayResult final {
    bool ok{false};
    QString message;

    [[nodiscard]] static GatewayResult Success(QString message = {}) {
        return {.ok = true, .message = std::move(message)};
    }

    [[nodiscard]] static GatewayResult Failure(QString message) {
        return {.ok = false, .message = std::move(message)};
    }
};

enum class WorkspaceUpdateType {
    kCreated,
    kSnapshotLoaded,
    kSnapshotSaved,
    kOperationApplied,
    kDeleted,
};

struct WorkspaceListItem final {
    QString workspace_id;
    QString name;
    QString owner_display_name;
    QDateTime updated_at;
    bool shared{false};
    int active_users{0};
};

struct WorkspaceShareInvite final {
    QString workspace_id;
    QString invite_code;
    QString invite_url;
    QDateTime expires_at;
};

struct WorkspacePresenceUser final {
    QString user_id;
    QString display_name;
    QDateTime joined_at;
};

struct WorkspaceCheckpoint final {
    QString checkpoint_id;
    QString workspace_id;
    QString name;
    QDateTime created_at;
    int revision{0};
};

enum class DroneHealth {
    kNominal,
    kWarning,
    kCritical,
    kOffline,
};

struct DroneTelemetry final {
    QString drone_id;
    QString callsign;
    double latitude_deg{0.0};
    double longitude_deg{0.0};
    double altitude_m{0.0};
    double heading_deg{0.0};
    double velocity_mps{0.0};
    double battery_percent{0.0};
    DroneHealth health{DroneHealth::kNominal};
    QDateTime timestamp;
};

struct DroneAgentEndpoint final {
    QString agent_id;
    QString display_name;
    QString ip_address;
    bool available{true};
};

struct SwarmStateSnapshot final {
    QString swarm_id;
    QList<DroneTelemetry> drones;
    QDateTime timestamp;
};

enum class MissionSimulationState {
    kIdle,
    kRunning,
    kPaused,
    kCompleted,
    kRejected,
};

struct SimulatedDronePosition final {
    QString node_id{};
    QString drone_id{};
    QString from_node_id{};
    QString to_node_id{};
    QPointF position{};
    int edge_pass_index{0};
    bool landed{false};
};

struct SimulatedDroneTrailSegment final {
    QString drone_id{};
    QString from_node_id{};
    QString to_node_id{};
    QPointF start_position{};
    QPointF end_position{};
    int edge_pass_index{0};
};

struct MissionSimulationFrame final {
    QString workspace_id{};
    MissionSimulationState state{MissionSimulationState::kIdle};
    QList<SimulatedDronePosition> drones{};
    QList<SimulatedDroneTrailSegment> trail_segments{};
    QString message{};
    int frame_index{0};
};

enum class SwarmCommandKind {
    kArm,
    kDisarm,
    kTakeoff,
    kLand,
    kHold,
    kReturnHome,
    kCustom,
};

struct SwarmCommand final {
    QString command_id;
    QString swarm_id;
    QString target_drone_id;
    SwarmCommandKind kind{SwarmCommandKind::kCustom};
    QVariantMap parameters;
};

struct CommandReceipt final {
    QString command_id;
    bool accepted{false};
    QString message;
    QDateTime accepted_at;
};

enum class AlertSeverity {
    kInfo,
    kWarning,
    kCritical,
};

struct AlertEvent final {
    QString alert_id;
    QString workspace_id;
    QString swarm_id;
    AlertSeverity severity{AlertSeverity::kInfo};
    QString title;
    QString message;
    QDateTime timestamp;
};

}  // namespace app::client_gateway
