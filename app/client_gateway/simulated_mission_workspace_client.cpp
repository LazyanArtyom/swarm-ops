#include "app/client_gateway/simulated_mission_workspace_client.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QUuid>
#include <algorithm>
#include <utility>

namespace app::client_gateway {
namespace {

QString NewId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QDateTime NowUtc() {
    return QDateTime::currentDateTimeUtc();
}

QString NewInviteCode() {
    constexpr int kCodeDigits = 100000;
    return QStringLiteral("SWARM-%1")
        .arg(QRandomGenerator::global()->bounded(kCodeDigits, kCodeDigits * 10));
}

QString NodeTypeToString(mission::GraphNodeType type) {
    switch (type) {
        case mission::GraphNodeType::kGeneric:
            return QStringLiteral("generic");
        case mission::GraphNodeType::kBorder:
            return QStringLiteral("border");
        case mission::GraphNodeType::kCorner:
            return QStringLiteral("corner");
    }
    return QStringLiteral("generic");
}

mission::GraphNodeType NodeTypeFromString(const QString& value) {
    if (value == QStringLiteral("border")) {
        return mission::GraphNodeType::kBorder;
    }
    if (value == QStringLiteral("corner")) {
        return mission::GraphNodeType::kCorner;
    }
    return mission::GraphNodeType::kGeneric;
}

QString NodeCategoryToString(mission::GraphNodeCategory category) {
    switch (category) {
        case mission::GraphNodeCategory::kGeneric:
            return QStringLiteral("generic");
        case mission::GraphNodeCategory::kDrone:
            return QStringLiteral("drone");
        case mission::GraphNodeCategory::kAttacker:
            return QStringLiteral("attacker");
        case mission::GraphNodeCategory::kTarget:
            return QStringLiteral("target");
    }
    return QStringLiteral("generic");
}

mission::GraphNodeCategory NodeCategoryFromString(const QString& value) {
    if (value == QStringLiteral("drone")) {
        return mission::GraphNodeCategory::kDrone;
    }
    if (value == QStringLiteral("attacker")) {
        return mission::GraphNodeCategory::kAttacker;
    }
    if (value == QStringLiteral("target")) {
        return mission::GraphNodeCategory::kTarget;
    }
    return mission::GraphNodeCategory::kGeneric;
}

QString ImageToBase64Png(const QImage& image) {
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return QString::fromLatin1(bytes.toBase64());
}

QImage ImageFromBase64Png(const QString& encoded) {
    return QImage::fromData(QByteArray::fromBase64(encoded.toLatin1()), "PNG");
}

QJsonObject BoundsToJson(const mission::MapBounds& bounds) {
    return {
        {QStringLiteral("north"), bounds.north},
        {QStringLiteral("west"), bounds.west},
        {QStringLiteral("south"), bounds.south},
        {QStringLiteral("east"), bounds.east},
    };
}

mission::MapBounds BoundsFromJson(const QJsonObject& object) {
    return {
        .north = object.value(QStringLiteral("north")).toDouble(),
        .west = object.value(QStringLiteral("west")).toDouble(),
        .south = object.value(QStringLiteral("south")).toDouble(),
        .east = object.value(QStringLiteral("east")).toDouble(),
    };
}

bool ContainsNode(const mission::MissionWorkspace& workspace, const QString& node_id) {
    return std::any_of(workspace.nodes.begin(), workspace.nodes.end(),
                       [&node_id](const mission::GraphNode& node) {
                           return node.id == node_id;
                       });
}

QJsonObject WorkspaceToJson(const mission::MissionWorkspace& workspace) {
    QJsonArray nodes;
    for (const mission::GraphNode& node : workspace.nodes) {
        nodes.push_back(QJsonObject{
            {QStringLiteral("id"), node.id},
            {QStringLiteral("label"), node.label},
            {QStringLiteral("x"), node.position.x()},
            {QStringLiteral("y"), node.position.y()},
            {QStringLiteral("type"), NodeTypeToString(node.type)},
            {QStringLiteral("category"), NodeCategoryToString(node.category)},
        });
    }

    QJsonArray edges;
    for (const mission::GraphEdge& edge : workspace.edges) {
        edges.push_back(QJsonObject{
            {QStringLiteral("id"), edge.id},
            {QStringLiteral("fromNodeId"), edge.from_node_id},
            {QStringLiteral("toNodeId"), edge.to_node_id},
        });
    }

    return {
        {QStringLiteral("id"), workspace.id},
        {QStringLiteral("name"), workspace.name},
        {QStringLiteral("background"),
         QJsonObject{
             {QStringLiteral("imagePngBase64"), ImageToBase64Png(workspace.background.image)},
             {QStringLiteral("bounds"), BoundsToJson(workspace.background.bounds)},
         }},
        {QStringLiteral("nodes"), nodes},
        {QStringLiteral("edges"), edges},
    };
}

mission::MissionWorkspace WorkspaceFromJson(const QJsonObject& object) {
    mission::MissionWorkspace workspace;
    workspace.id = object.value(QStringLiteral("id")).toString();
    workspace.name = object.value(QStringLiteral("name")).toString();

    const QJsonObject background = object.value(QStringLiteral("background")).toObject();
    workspace.background.image =
        ImageFromBase64Png(background.value(QStringLiteral("imagePngBase64")).toString());
    workspace.background.bounds = BoundsFromJson(background.value(QStringLiteral("bounds")).toObject());

    const QJsonArray nodes = object.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& value : nodes) {
        const QJsonObject node_object = value.toObject();
        mission::GraphNode node;
        node.id = node_object.value(QStringLiteral("id")).toString();
        node.label = node_object.value(QStringLiteral("label")).toString();
        node.position = QPointF(node_object.value(QStringLiteral("x")).toDouble(),
                                node_object.value(QStringLiteral("y")).toDouble());
        node.type = NodeTypeFromString(node_object.value(QStringLiteral("type")).toString());
        node.category =
            NodeCategoryFromString(node_object.value(QStringLiteral("category")).toString());
        if (!node.id.isEmpty()) {
            workspace.nodes.push_back(node);
        }
    }

    const QJsonArray edges = object.value(QStringLiteral("edges")).toArray();
    for (const QJsonValue& value : edges) {
        const QJsonObject edge_object = value.toObject();
        mission::GraphEdge edge;
        edge.id = edge_object.value(QStringLiteral("id")).toString();
        edge.from_node_id = edge_object.value(QStringLiteral("fromNodeId")).toString();
        edge.to_node_id = edge_object.value(QStringLiteral("toNodeId")).toString();
        if (!edge.id.isEmpty() && ContainsNode(workspace, edge.from_node_id) &&
            ContainsNode(workspace, edge.to_node_id)) {
            workspace.edges.push_back(edge);
        }
    }

    return workspace;
}

}  // namespace

SimulatedMissionWorkspaceClient::SimulatedMissionWorkspaceClient(QObject* parent)
    : IMissionWorkspaceClient(parent) {}

void SimulatedMissionWorkspaceClient::Start() {
    if (state_ == GatewayConnectionState::kConnected) {
        return;
    }

    state_ = GatewayConnectionState::kConnecting;
    emit SigConnectionChanged(state_);
    LoadStore();
    state_ = GatewayConnectionState::kConnected;
    emit SigConnectionChanged(state_);
    EmitWorkspaceList();
}

void SimulatedMissionWorkspaceClient::Stop() {
    if (state_ == GatewayConnectionState::kDisconnected) {
        return;
    }

    LeaveWorkspaceSession();
    state_ = GatewayConnectionState::kDisconnected;
    emit SigConnectionChanged(state_);
}

GatewayConnectionState SimulatedMissionWorkspaceClient::ConnectionState() const {
    return state_;
}

QList<WorkspaceListItem> SimulatedMissionWorkspaceClient::ListWorkspaces() const {
    QList<WorkspaceListItem> items;
    items.reserve(workspaces_.size());
    for (const StoredWorkspace& stored : workspaces_) {
        if (!stored.saved) {
            continue;
        }
        items.push_back(ToListItem(stored));
    }
    std::sort(items.begin(), items.end(), [](const WorkspaceListItem& lhs,
                                             const WorkspaceListItem& rhs) {
        return lhs.updated_at > rhs.updated_at;
    });
    return items;
}

mission::MissionWorkspace SimulatedMissionWorkspaceClient::ActiveWorkspace() const {
    return workspaces_.value(active_workspace_id_).workspace;
}

mission::MissionWorkspace SimulatedMissionWorkspaceClient::GetWorkspaceSnapshot(
    const QString& workspace_id) const {
    return workspaces_.value(workspace_id).workspace;
}

QString SimulatedMissionWorkspaceClient::CreateWorkspace(
    QString name, const mission::WorkspaceBackground& background) {
    EnsureConnected();

    mission::MissionWorkspace workspace;
    workspace.id = NewId();
    workspace.name = name.trimmed().isEmpty() ? tr("Mission %1").arg(next_workspace_number_++)
                                              : name.trimmed();
    workspace.background = background;

    StoredWorkspace stored;
    stored.workspace = workspace;
    stored.owner_display_name = tr("Local Operator");
    stored.created_at = NowUtc();
    stored.updated_at = stored.created_at;
    stored.revision = next_revision_++;
    stored.shared = false;
    stored.saved = false;

    active_workspace_id_ = workspace.id;
    workspaces_.insert(workspace.id, stored);
    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged(workspace, WorkspaceUpdateType::kCreated);
    return workspace.id;
}

GatewayResult SimulatedMissionWorkspaceClient::SaveWorkspaceSnapshot(
    mission::MissionWorkspace workspace) {
    if (workspace.id.isEmpty()) {
        return GatewayResult::Failure(tr("Workspace id is empty."));
    }

    EnsureConnected();
    StoredWorkspace stored = workspaces_.value(workspace.id);
    if (stored.created_at.isNull()) {
        stored.created_at = NowUtc();
        stored.owner_display_name = tr("Local Operator");
        stored.shared = false;
    }
    stored.workspace = std::move(workspace);
    stored.updated_at = NowUtc();
    stored.revision = next_revision_++;
    stored.saved = true;
    active_workspace_id_ = stored.workspace.id;
    workspaces_.insert(stored.workspace.id, stored);

    QString error_message;
    if (!SaveStore(&error_message)) {
        return GatewayResult::Failure(error_message);
    }

    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged(stored.workspace, WorkspaceUpdateType::kSnapshotSaved);
    return GatewayResult::Success(tr("Workspace saved in simulator."));
}

GatewayResult SimulatedMissionWorkspaceClient::DeleteWorkspace(const QString& workspace_id) {
    if (!workspaces_.contains(workspace_id)) {
        return GatewayResult::Failure(tr("Workspace does not exist."));
    }

    workspaces_.remove(workspace_id);
    checkpoints_.remove(workspace_id);
    presence_.remove(workspace_id);
    if (active_workspace_id_ == workspace_id) {
        active_workspace_id_.clear();
    }
    QString error_message;
    if (!SaveStore(&error_message)) {
        return GatewayResult::Failure(error_message);
    }
    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged({}, WorkspaceUpdateType::kDeleted);
    return GatewayResult::Success(tr("Workspace deleted from simulator."));
}

GatewayResult SimulatedMissionWorkspaceClient::SetWorkspaceShared(const QString& workspace_id,
                                                                  bool shared) {
    if (!workspaces_.contains(workspace_id)) {
        return GatewayResult::Failure(tr("Workspace does not exist."));
    }

    StoredWorkspace stored = workspaces_.value(workspace_id);
    if (stored.shared == shared) {
        return GatewayResult::Success(shared ? tr("Workspace is already shared.")
                                             : tr("Workspace is already private."));
    }

    stored.shared = shared;
    stored.updated_at = NowUtc();
    stored.revision = next_revision_++;
    stored.saved = true;
    workspaces_.insert(workspace_id, stored);
    QString error_message;
    if (!SaveStore(&error_message)) {
        return GatewayResult::Failure(error_message);
    }
    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged(stored.workspace, WorkspaceUpdateType::kOperationApplied);
    return GatewayResult::Success(shared ? tr("Workspace sharing enabled.")
                                         : tr("Workspace sharing disabled."));
}

WorkspaceShareInvite SimulatedMissionWorkspaceClient::CreateWorkspaceInvite(
    const QString& workspace_id) {
    WorkspaceShareInvite invite;
    if (!workspaces_.contains(workspace_id)) {
        return invite;
    }

    (void)SetWorkspaceShared(workspace_id, true);
    invite.workspace_id = workspace_id;
    invite.invite_code = NewInviteCode();
    invite.invite_url = QStringLiteral("swarmops://workspace/%1?invite=%2")
                            .arg(workspace_id, invite.invite_code);
    invite.expires_at = NowUtc().addDays(7);
    return invite;
}

GatewayResult SimulatedMissionWorkspaceClient::JoinWorkspaceSession(const QString& workspace_id,
                                                                    QString user_display_name) {
    if (!workspaces_.contains(workspace_id)) {
        return GatewayResult::Failure(tr("Workspace does not exist."));
    }

    active_workspace_id_ = workspace_id;
    WorkspacePresenceUser local_user;
    local_user.user_id = QStringLiteral("local-operator");
    local_user.display_name =
        user_display_name.trimmed().isEmpty() ? tr("Local Operator") : user_display_name.trimmed();
    local_user.joined_at = NowUtc();

    auto users = presence_.value(workspace_id);
    users.erase(std::remove_if(users.begin(), users.end(),
                               [&local_user](const WorkspacePresenceUser& user) {
                                   return user.user_id == local_user.user_id;
                               }),
                users.end());
    users.push_back(local_user);
    presence_.insert(workspace_id, users);

    EmitWorkspaceList();
    EmitPresence(workspace_id);
    emit SigWorkspaceSnapshotChanged(workspaces_.value(workspace_id).workspace,
                                     WorkspaceUpdateType::kSnapshotLoaded);
    return GatewayResult::Success(tr("Joined workspace session."));
}

void SimulatedMissionWorkspaceClient::LeaveWorkspaceSession() {
    if (active_workspace_id_.isEmpty()) {
        return;
    }

    const QString workspace_id = active_workspace_id_;
    active_workspace_id_.clear();
    EmitPresence(workspace_id);
}

GatewayResult SimulatedMissionWorkspaceClient::ApplyWorkspaceSnapshot(
    mission::MissionWorkspace workspace, WorkspaceUpdateType update_type) {
    if (workspace.id.isEmpty()) {
        return GatewayResult::Failure(tr("Workspace id is empty."));
    }

    EnsureConnected();
    StoredWorkspace stored = workspaces_.value(workspace.id);
    if (stored.created_at.isNull()) {
        stored.created_at = NowUtc();
        stored.owner_display_name = tr("Local Operator");
        stored.shared = false;
    }
    stored.workspace = std::move(workspace);
    stored.updated_at = NowUtc();
    stored.revision = next_revision_++;
    active_workspace_id_ = stored.workspace.id;
    workspaces_.insert(stored.workspace.id, stored);
    if (stored.saved) {
        QString error_message;
        if (!SaveStore(&error_message)) {
            return GatewayResult::Failure(error_message);
        }
    }

    EmitWorkspaceList();
    emit SigWorkspaceSnapshotChanged(stored.workspace, update_type);
    return GatewayResult::Success(tr("Workspace operation applied."));
}

WorkspaceCheckpoint SimulatedMissionWorkspaceClient::CreateCheckpoint(const QString& workspace_id,
                                                                      QString name) {
    WorkspaceCheckpoint checkpoint;
    if (!workspaces_.contains(workspace_id)) {
        return checkpoint;
    }

    checkpoint.checkpoint_id = NewId();
    checkpoint.workspace_id = workspace_id;
    checkpoint.name = name.trimmed().isEmpty() ? tr("Checkpoint") : name.trimmed();
    checkpoint.created_at = NowUtc();
    checkpoint.revision = workspaces_.value(workspace_id).revision;
    checkpoints_[workspace_id].push_back(checkpoint);
    return checkpoint;
}

WorkspaceListItem SimulatedMissionWorkspaceClient::ToListItem(
    const StoredWorkspace& stored) const {
    return {
        .workspace_id = stored.workspace.id,
        .name = stored.workspace.name,
        .owner_display_name = stored.owner_display_name,
        .updated_at = stored.updated_at,
        .shared = stored.shared,
        .active_users = static_cast<int>(presence_.value(stored.workspace.id).size()),
    };
}

QList<WorkspacePresenceUser> SimulatedMissionWorkspaceClient::PresenceFor(
    const QString& workspace_id) const {
    return presence_.value(workspace_id);
}

QString SimulatedMissionWorkspaceClient::StoreFilePath() const {
    const QString app_data_dir =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QDir dir(app_data_dir.isEmpty() ? QDir::currentPath() : app_data_dir);
    return dir.filePath(QStringLiteral("simulator_workspaces.json"));
}

bool SimulatedMissionWorkspaceClient::SaveStore(QString* error_message) const {
    QJsonArray workspaces;
    for (const StoredWorkspace& stored : workspaces_) {
        if (!stored.saved) {
            continue;
        }

        workspaces.push_back(QJsonObject{
            {QStringLiteral("workspace"), WorkspaceToJson(stored.workspace)},
            {QStringLiteral("ownerDisplayName"), stored.owner_display_name},
            {QStringLiteral("createdAt"), stored.created_at.toUTC().toString(Qt::ISODateWithMs)},
            {QStringLiteral("updatedAt"), stored.updated_at.toUTC().toString(Qt::ISODateWithMs)},
            {QStringLiteral("revision"), stored.revision},
            {QStringLiteral("shared"), stored.shared},
            {QStringLiteral("saved"), stored.saved},
        });
    }

    const QString file_path = StoreFilePath();
    QDir dir(QFileInfo(file_path).absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (error_message != nullptr) {
            *error_message = tr("Could not create simulator workspace store.");
        }
        return false;
    }

    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error_message != nullptr) {
            *error_message = tr("Could not open simulator workspace store for writing.");
        }
        return false;
    }

    const QJsonObject root{
        {QStringLiteral("format"), QStringLiteral("swarmops.simulator.workspaces")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("nextWorkspaceNumber"), next_workspace_number_},
        {QStringLiteral("nextRevision"), next_revision_},
        {QStringLiteral("workspaces"), workspaces},
    };
    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size()) {
        if (error_message != nullptr) {
            *error_message = tr("Could not write simulator workspace store.");
        }
        return false;
    }

    return true;
}

void SimulatedMissionWorkspaceClient::LoadStore() {
    if (store_loaded_) {
        return;
    }
    store_loaded_ = true;

    QFile file(StoreFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        return;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() !=
        QStringLiteral("swarmops.simulator.workspaces")) {
        return;
    }

    workspaces_.clear();
    next_workspace_number_ = std::max(1, root.value(QStringLiteral("nextWorkspaceNumber")).toInt(1));
    next_revision_ = std::max(1, root.value(QStringLiteral("nextRevision")).toInt(1));

    const QJsonArray stored_workspaces = root.value(QStringLiteral("workspaces")).toArray();
    for (const QJsonValue& value : stored_workspaces) {
        const QJsonObject object = value.toObject();
        StoredWorkspace stored;
        stored.workspace = WorkspaceFromJson(object.value(QStringLiteral("workspace")).toObject());
        if (stored.workspace.id.isEmpty()) {
            continue;
        }

        stored.owner_display_name = object.value(QStringLiteral("ownerDisplayName"))
                                        .toString(tr("Local Operator"));
        stored.created_at =
            QDateTime::fromString(object.value(QStringLiteral("createdAt")).toString(),
                                  Qt::ISODateWithMs);
        stored.updated_at =
            QDateTime::fromString(object.value(QStringLiteral("updatedAt")).toString(),
                                  Qt::ISODateWithMs);
        stored.revision = object.value(QStringLiteral("revision")).toInt(next_revision_++);
        stored.shared = object.value(QStringLiteral("shared")).toBool(false);
        stored.saved = object.value(QStringLiteral("saved")).toBool(true);
        if (stored.created_at.isNull()) {
            stored.created_at = NowUtc();
        }
        if (stored.updated_at.isNull()) {
            stored.updated_at = stored.created_at;
        }

        next_revision_ = std::max(next_revision_, stored.revision + 1);
        workspaces_.insert(stored.workspace.id, stored);
    }
}

void SimulatedMissionWorkspaceClient::EmitWorkspaceList() {
    emit SigWorkspaceListChanged(ListWorkspaces());
}

void SimulatedMissionWorkspaceClient::EmitPresence(const QString& workspace_id) {
    emit SigWorkspacePresenceChanged(workspace_id, PresenceFor(workspace_id));
}

void SimulatedMissionWorkspaceClient::EnsureConnected() {
    if (state_ != GatewayConnectionState::kConnected) {
        Start();
    }
}

}  // namespace app::client_gateway
