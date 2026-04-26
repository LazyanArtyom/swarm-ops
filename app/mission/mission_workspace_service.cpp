#include "app/mission/mission_workspace_service.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <algorithm>

#include "app/mission/in_memory_mission_workspace_gateway.h"
#include "app/mission/mission_workspace_gateway.h"

namespace app::mission {

namespace {

QString NewId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool ContainsNode(const MissionWorkspace& workspace, const QString& node_id) {
    return std::any_of(workspace.nodes.begin(), workspace.nodes.end(),
                       [&node_id](const GraphNode& node) { return node.id == node_id; });
}

bool HasEdge(const MissionWorkspace& workspace, const QString& from_node_id, const QString& to_node_id) {
    return std::any_of(workspace.edges.begin(), workspace.edges.end(),
                       [&from_node_id, &to_node_id](const GraphEdge& edge) {
                           return (edge.from_node_id == from_node_id &&
                                   edge.to_node_id == to_node_id) ||
                                  (edge.from_node_id == to_node_id &&
                                   edge.to_node_id == from_node_id);
                       });
}

QString NodeTypeToString(GraphNodeType type) {
    switch (type) {
        case GraphNodeType::kBorder:
            return QStringLiteral("border");
        case GraphNodeType::kCorner:
            return QStringLiteral("corner");
    }
    return QStringLiteral("border");
}

GraphNodeType NodeTypeFromString(const QString& value) {
    if (value == QStringLiteral("corner")) {
        return GraphNodeType::kCorner;
    }
    return GraphNodeType::kBorder;
}

QString NodeCategoryToString(GraphNodeCategory category) {
    switch (category) {
        case GraphNodeCategory::kGeneric:
            return QStringLiteral("generic");
        case GraphNodeCategory::kDrone:
            return QStringLiteral("drone");
        case GraphNodeCategory::kAttacker:
            return QStringLiteral("attacker");
        case GraphNodeCategory::kTarget:
            return QStringLiteral("target");
    }
    return QStringLiteral("generic");
}

GraphNodeCategory NodeCategoryFromString(const QString& value) {
    if (value == QStringLiteral("drone")) {
        return GraphNodeCategory::kDrone;
    }
    if (value == QStringLiteral("attacker")) {
        return GraphNodeCategory::kAttacker;
    }
    if (value == QStringLiteral("target")) {
        return GraphNodeCategory::kTarget;
    }
    return GraphNodeCategory::kGeneric;
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

QJsonObject BoundsToJson(const MapBounds& bounds) {
    return {
        {QStringLiteral("north"), bounds.north},
        {QStringLiteral("west"), bounds.west},
        {QStringLiteral("south"), bounds.south},
        {QStringLiteral("east"), bounds.east},
    };
}

MapBounds BoundsFromJson(const QJsonObject& object) {
    return {
        .north = object.value(QStringLiteral("north")).toDouble(),
        .west = object.value(QStringLiteral("west")).toDouble(),
        .south = object.value(QStringLiteral("south")).toDouble(),
        .east = object.value(QStringLiteral("east")).toDouble(),
    };
}

QJsonObject WorkspaceToJson(const MissionWorkspace& workspace) {
    QJsonArray nodes;
    for (const GraphNode& node : workspace.nodes) {
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
    for (const GraphEdge& edge : workspace.edges) {
        edges.push_back(QJsonObject{
            {QStringLiteral("id"), edge.id},
            {QStringLiteral("fromNodeId"), edge.from_node_id},
            {QStringLiteral("toNodeId"), edge.to_node_id},
        });
    }

    return {
        {QStringLiteral("format"), QStringLiteral("swarmops.workspace")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("workspace"),
         QJsonObject{
             {QStringLiteral("id"), workspace.id},
             {QStringLiteral("name"), workspace.name},
             {QStringLiteral("background"),
              QJsonObject{
                  {QStringLiteral("imagePngBase64"), ImageToBase64Png(workspace.background.image)},
                  {QStringLiteral("bounds"), BoundsToJson(workspace.background.bounds)},
              }},
             {QStringLiteral("nodes"), nodes},
             {QStringLiteral("edges"), edges},
         }},
    };
}

bool WorkspaceFromJson(const QJsonObject& root, MissionWorkspace* workspace, QString* error_message) {
    if (workspace == nullptr) {
        return false;
    }
    if (root.value(QStringLiteral("format")).toString() != QStringLiteral("swarmops.workspace")) {
        if (error_message != nullptr) {
            *error_message = QObject::tr("The selected file is not a SwarmOps workspace.");
        }
        return false;
    }

    const QJsonObject workspace_object = root.value(QStringLiteral("workspace")).toObject();
    MissionWorkspace parsed;
    parsed.id = workspace_object.value(QStringLiteral("id")).toString();
    parsed.name = workspace_object.value(QStringLiteral("name")).toString();
    if (parsed.id.isEmpty()) {
        parsed.id = NewId();
    }
    if (parsed.name.isEmpty()) {
        parsed.name = QObject::tr("Untitled");
    }

    const QJsonObject background_object = workspace_object.value(QStringLiteral("background")).toObject();
    parsed.background.image =
        ImageFromBase64Png(background_object.value(QStringLiteral("imagePngBase64")).toString());
    parsed.background.bounds = BoundsFromJson(background_object.value(QStringLiteral("bounds")).toObject());

    const QJsonArray nodes = workspace_object.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& value : nodes) {
        const QJsonObject object = value.toObject();
        GraphNode node;
        node.id = object.value(QStringLiteral("id")).toString();
        node.label = object.value(QStringLiteral("label")).toString();
        node.position = QPointF(object.value(QStringLiteral("x")).toDouble(),
                                object.value(QStringLiteral("y")).toDouble());
        node.type = NodeTypeFromString(object.value(QStringLiteral("type")).toString());
        node.category = NodeCategoryFromString(object.value(QStringLiteral("category")).toString());
        if (!node.id.isEmpty()) {
            parsed.nodes.push_back(node);
        }
    }

    const QJsonArray edges = workspace_object.value(QStringLiteral("edges")).toArray();
    for (const QJsonValue& value : edges) {
        const QJsonObject object = value.toObject();
        GraphEdge edge;
        edge.id = object.value(QStringLiteral("id")).toString();
        edge.from_node_id = object.value(QStringLiteral("fromNodeId")).toString();
        edge.to_node_id = object.value(QStringLiteral("toNodeId")).toString();
        if (!edge.id.isEmpty() && ContainsNode(parsed, edge.from_node_id) &&
            ContainsNode(parsed, edge.to_node_id)) {
            parsed.edges.push_back(edge);
        }
    }

    *workspace = std::move(parsed);
    return true;
}

}  // namespace

MissionWorkspaceService::MissionWorkspaceService(std::unique_ptr<IMissionWorkspaceGateway> gateway,
                                                 QObject* parent)
    : QObject(parent), gateway_(std::move(gateway)) {
    if (gateway_ != nullptr) {
        connect(gateway_.get(), &IMissionWorkspaceGateway::SigWorkspaceChanged, this,
                &MissionWorkspaceService::SigWorkspaceChanged);
    }
}

MissionWorkspace MissionWorkspaceService::ActiveWorkspace() const {
    return gateway_ != nullptr ? gateway_->ActiveWorkspace() : MissionWorkspace{};
}

bool MissionWorkspaceService::HasActiveWorkspace() const {
    return !ActiveWorkspace().id.isEmpty();
}

QString MissionWorkspaceService::WorkspaceFilePath() const {
    return workspace_file_path_;
}

QString MissionWorkspaceService::WorkspaceDisplayName() const {
    if (!workspace_file_path_.isEmpty()) {
        return QFileInfo(workspace_file_path_).completeBaseName();
    }
    return tr("Untitled");
}

bool MissionWorkspaceService::IsDirty() const {
    return dirty_;
}

void MissionWorkspaceService::NewWorkspace() {
    undo_stack_.clear();
    redo_stack_.clear();
    Restore(UntitledWorkspace());
    SetDocumentState(QString(), false);
}

bool MissionWorkspaceService::LoadWorkspace(const QString& file_path, QString* error_message) {
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error_message != nullptr) {
            *error_message = tr("Could not open %1.").arg(QFileInfo(file_path).fileName());
        }
        return false;
    }

    QJsonParseError mutable_parse_error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &mutable_parse_error);
    if (mutable_parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        if (error_message != nullptr) {
            *error_message = tr("Could not read workspace file: %1.")
                                 .arg(mutable_parse_error.errorString());
        }
        return false;
    }

    MissionWorkspace workspace;
    if (!WorkspaceFromJson(document.object(), &workspace, error_message)) {
        return false;
    }

    undo_stack_.clear();
    redo_stack_.clear();
    Restore(std::move(workspace));
    SetDocumentState(file_path, false);
    RequestGraphEditor();
    return true;
}

bool MissionWorkspaceService::SaveWorkspace(QString* error_message) {
    if (workspace_file_path_.isEmpty()) {
        if (error_message != nullptr) {
            *error_message = tr("Workspace does not have a save location.");
        }
        return false;
    }
    return SaveWorkspaceAs(workspace_file_path_, error_message);
}

bool MissionWorkspaceService::SaveWorkspaceAs(const QString& file_path, QString* error_message) {
    MissionWorkspace workspace = ActiveWorkspace();
    if (workspace.id.isEmpty()) {
        workspace = UntitledWorkspace();
    }
    if (workspace.name.isEmpty() || workspace.name == tr("Untitled")) {
        workspace.name = QFileInfo(file_path).completeBaseName();
    }

    QFile file(file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error_message != nullptr) {
            *error_message = tr("Could not save %1.").arg(QFileInfo(file_path).fileName());
        }
        return false;
    }

    const QByteArray payload =
        QJsonDocument(WorkspaceToJson(workspace)).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size()) {
        if (error_message != nullptr) {
            *error_message = tr("Could not write all workspace data to %1.")
                                 .arg(QFileInfo(file_path).fileName());
        }
        return false;
    }

    Restore(workspace);
    SetDocumentState(file_path, false);
    return true;
}

void MissionWorkspaceService::CreateWorkspaceFromImage(const QImage& image) {
    if (image.isNull()) {
        return;
    }

    WorkspaceBackground background;
    background.image = image;
    CreateWorkspaceFromMapCapture(background);
}

void MissionWorkspaceService::CreateWorkspaceFromMapCapture(const WorkspaceBackground& background) {
    if (gateway_ == nullptr || !background.IsValid()) {
        return;
    }

    (void)gateway_->CreateWorkspace(background);
    undo_stack_.clear();
    redo_stack_.clear();
    MarkDirty();
    RequestGraphEditor();
}

void MissionWorkspaceService::AddNode(QPointF position) {
    MissionWorkspace workspace = ActiveWorkspace();
    if (workspace.id.isEmpty()) {
        return;
    }

    GraphNode node;
    node.id = NewId();
    node.label = NextNodeLabel(workspace);
    node.position = position;
    workspace.nodes.push_back(node);
    Commit(workspace);
}

void MissionWorkspaceService::MoveNode(const QString& node_id, QPointF position) {
    MissionWorkspace workspace = ActiveWorkspace();
    for (GraphNode& node : workspace.nodes) {
        if (node.id == node_id) {
            if (node.position == position) {
                return;
            }
            node.position = position;
            Commit(workspace);
            return;
        }
    }
}

void MissionWorkspaceService::MoveNodes(const QHash<QString, QPointF>& node_positions) {
    if (node_positions.isEmpty()) {
        return;
    }

    MissionWorkspace workspace = ActiveWorkspace();
    bool changed = false;
    for (GraphNode& node : workspace.nodes) {
        const auto position_it = node_positions.constFind(node.id);
        if (position_it == node_positions.constEnd() || node.position == position_it.value()) {
            continue;
        }
        node.position = position_it.value();
        changed = true;
    }

    if (changed) {
        Commit(workspace);
    }
}

void MissionWorkspaceService::RemoveNode(const QString& node_id) {
    MissionWorkspace workspace = ActiveWorkspace();
    const qsizetype node_count = workspace.nodes.size();
    const qsizetype edge_count = workspace.edges.size();
    const auto node_matches = [&node_id](const GraphNode& node) { return node.id == node_id; };
    const auto edge_matches = [&node_id](const GraphEdge& edge) {
        return edge.from_node_id == node_id || edge.to_node_id == node_id;
    };

    workspace.nodes.erase(std::remove_if(workspace.nodes.begin(), workspace.nodes.end(), node_matches),
                          workspace.nodes.end());
    workspace.edges.erase(std::remove_if(workspace.edges.begin(), workspace.edges.end(), edge_matches),
                          workspace.edges.end());
    if (workspace.nodes.size() == node_count && workspace.edges.size() == edge_count) {
        return;
    }
    Commit(workspace);
}

void MissionWorkspaceService::RemoveItems(const QList<QString>& node_ids, const QList<QString>& edge_ids) {
    if (node_ids.isEmpty() && edge_ids.isEmpty()) {
        return;
    }

    MissionWorkspace workspace = ActiveWorkspace();
    const qsizetype node_count = workspace.nodes.size();
    const qsizetype edge_count = workspace.edges.size();

    workspace.nodes.erase(
        std::remove_if(workspace.nodes.begin(), workspace.nodes.end(),
                       [&node_ids](const GraphNode& node) { return node_ids.contains(node.id); }),
        workspace.nodes.end());

    workspace.edges.erase(
        std::remove_if(workspace.edges.begin(), workspace.edges.end(),
                       [&node_ids, &edge_ids](const GraphEdge& edge) {
                           return edge_ids.contains(edge.id) || node_ids.contains(edge.from_node_id) ||
                                  node_ids.contains(edge.to_node_id);
                       }),
        workspace.edges.end());

    if (workspace.nodes.size() == node_count && workspace.edges.size() == edge_count) {
        return;
    }
    Commit(workspace);
}

void MissionWorkspaceService::AddEdge(const QString& from_node_id, const QString& to_node_id) {
    if (from_node_id.isEmpty() || to_node_id.isEmpty() || from_node_id == to_node_id) {
        return;
    }

    MissionWorkspace workspace = ActiveWorkspace();
    if (!ContainsNode(workspace, from_node_id) || !ContainsNode(workspace, to_node_id) ||
        HasEdge(workspace, from_node_id, to_node_id)) {
        return;
    }

    GraphEdge edge;
    edge.id = NewId();
    edge.from_node_id = from_node_id;
    edge.to_node_id = to_node_id;
    workspace.edges.push_back(edge);
    Commit(workspace);
}

void MissionWorkspaceService::RemoveEdge(const QString& edge_id) {
    MissionWorkspace workspace = ActiveWorkspace();
    const qsizetype edge_count = workspace.edges.size();
    workspace.edges.erase(std::remove_if(workspace.edges.begin(), workspace.edges.end(),
                                         [&edge_id](const GraphEdge& edge) {
                                             return edge.id == edge_id;
                                         }),
                          workspace.edges.end());
    if (workspace.edges.size() == edge_count) {
        return;
    }
    Commit(workspace);
}

void MissionWorkspaceService::SetNodeLabel(const QString& node_id, const QString& label) {
    const QString trimmed_label = label.trimmed();
    if (trimmed_label.isEmpty()) {
        return;
    }

    MissionWorkspace workspace = ActiveWorkspace();
    for (GraphNode& node : workspace.nodes) {
        if (node.id == node_id) {
            if (node.label == trimmed_label) {
                return;
            }
            node.label = trimmed_label;
            Commit(workspace);
            return;
        }
    }
}

void MissionWorkspaceService::SetNodeType(const QString& node_id, GraphNodeType type) {
    MissionWorkspace workspace = ActiveWorkspace();
    for (GraphNode& node : workspace.nodes) {
        if (node.id == node_id) {
            if (node.type == type) {
                return;
            }
            node.type = type;
            Commit(workspace);
            return;
        }
    }
}

void MissionWorkspaceService::SetNodeCategory(const QString& node_id, GraphNodeCategory category) {
    MissionWorkspace workspace = ActiveWorkspace();
    for (GraphNode& node : workspace.nodes) {
        if (node.id == node_id) {
            if (node.category == category) {
                return;
            }
            node.category = category;
            Commit(workspace);
            return;
        }
    }
}

void MissionWorkspaceService::GenerateGrid(int rows, int columns) {
    MissionWorkspace workspace = ActiveWorkspace();
    if (workspace.id.isEmpty() || rows < 2 || columns < 2 || !workspace.background.IsValid()) {
        return;
    }

    workspace.nodes.clear();
    workspace.edges.clear();

    const QSize image_size = workspace.background.image.size();
    const double horizontal_padding = std::min(32.0, image_size.width() * 0.08);
    const double vertical_padding = std::min(32.0, image_size.height() * 0.08);
    const double usable_width = std::max(1.0, image_size.width() - (horizontal_padding * 2.0));
    const double usable_height = std::max(1.0, image_size.height() - (vertical_padding * 2.0));
    const double x_step = usable_width / static_cast<double>(columns - 1);
    const double y_step = usable_height / static_cast<double>(rows - 1);

    QList<QList<QString>> node_ids;
    node_ids.resize(rows);
    for (int row = 0; row < rows; ++row) {
        node_ids[row].resize(columns);
        for (int column = 0; column < columns; ++column) {
            GraphNode node;
            node.id = NewId();
            node.label = QStringLiteral("N%1:%2").arg(row).arg(column);
            node.position =
                QPointF(horizontal_padding + x_step * column, vertical_padding + y_step * row);
            node_ids[row][column] = node.id;
            workspace.nodes.push_back(node);
        }
    }

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            if (column + 1 < columns) {
                workspace.edges.push_back({NewId(), node_ids[row][column], node_ids[row][column + 1]});
            }
            if (row + 1 < rows) {
                workspace.edges.push_back({NewId(), node_ids[row][column], node_ids[row + 1][column]});
            }
        }
    }

    Commit(workspace);
}

void MissionWorkspaceService::RequestGraphEditor() {
    emit SigGraphEditorRequested();
}

void MissionWorkspaceService::Undo() {
    if (!CanUndo()) {
        return;
    }

    MissionWorkspace current = ActiveWorkspace();
    MissionWorkspace previous = undo_stack_.takeLast();
    if (!current.id.isEmpty()) {
        redo_stack_.push_back(current);
    }
    Restore(std::move(previous));
    MarkDirty();
}

void MissionWorkspaceService::Redo() {
    if (!CanRedo()) {
        return;
    }

    MissionWorkspace current = ActiveWorkspace();
    MissionWorkspace next = redo_stack_.takeLast();
    if (!current.id.isEmpty()) {
        undo_stack_.push_back(current);
    }
    Restore(std::move(next));
    MarkDirty();
}

bool MissionWorkspaceService::CanUndo() const {
    return !undo_stack_.isEmpty();
}

bool MissionWorkspaceService::CanRedo() const {
    return !redo_stack_.isEmpty();
}

QString MissionWorkspaceService::NextNodeLabel(const MissionWorkspace& workspace) const {
    return QStringLiteral("N%1").arg(workspace.nodes.size() + 1);
}

MissionWorkspace MissionWorkspaceService::UntitledWorkspace() const {
    MissionWorkspace workspace;
    workspace.id = NewId();
    workspace.name = tr("Untitled");
    return workspace;
}

void MissionWorkspaceService::Commit(MissionWorkspace workspace) {
    if (gateway_ != nullptr) {
        MissionWorkspace current = ActiveWorkspace();
        if (!current.id.isEmpty()) {
            undo_stack_.push_back(current);
            redo_stack_.clear();
        }
        gateway_->ReplaceWorkspace(std::move(workspace));
        MarkDirty();
    }
}

void MissionWorkspaceService::Restore(MissionWorkspace workspace) {
    if (gateway_ != nullptr) {
        gateway_->ReplaceWorkspace(std::move(workspace));
    }
}

void MissionWorkspaceService::SetDocumentState(QString file_path, bool dirty) {
    bool changed = workspace_file_path_ != file_path || dirty_ != dirty;
    workspace_file_path_ = std::move(file_path);
    dirty_ = dirty;
    if (changed) {
        emit SigDocumentStateChanged();
    }
}

void MissionWorkspaceService::MarkDirty() {
    if (!dirty_) {
        dirty_ = true;
        emit SigDocumentStateChanged();
    }
}

MissionWorkspaceService& MissionWorkspaceRuntime() {
    static auto* service = new MissionWorkspaceService(std::make_unique<InMemoryMissionWorkspaceGateway>());
    return *service;
}

}  // namespace app::mission
