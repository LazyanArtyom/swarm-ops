#include "app/mission/mission_workspace_service.h"

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

void MissionWorkspaceService::Commit(MissionWorkspace workspace) {
    if (gateway_ != nullptr) {
        MissionWorkspace current = ActiveWorkspace();
        if (!current.id.isEmpty()) {
            undo_stack_.push_back(current);
            redo_stack_.clear();
        }
        gateway_->ReplaceWorkspace(std::move(workspace));
    }
}

void MissionWorkspaceService::Restore(MissionWorkspace workspace) {
    if (gateway_ != nullptr) {
        gateway_->ReplaceWorkspace(std::move(workspace));
    }
}

MissionWorkspaceService& MissionWorkspaceRuntime() {
    static auto* service = new MissionWorkspaceService(std::make_unique<InMemoryMissionWorkspaceGateway>());
    return *service;
}

}  // namespace app::mission
