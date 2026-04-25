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
            node.position = position;
            Commit(workspace);
            return;
        }
    }
}

void MissionWorkspaceService::RemoveNode(const QString& node_id) {
    MissionWorkspace workspace = ActiveWorkspace();
    const auto node_matches = [&node_id](const GraphNode& node) { return node.id == node_id; };
    const auto edge_matches = [&node_id](const GraphEdge& edge) {
        return edge.from_node_id == node_id || edge.to_node_id == node_id;
    };

    workspace.nodes.erase(std::remove_if(workspace.nodes.begin(), workspace.nodes.end(), node_matches),
                          workspace.nodes.end());
    workspace.edges.erase(std::remove_if(workspace.edges.begin(), workspace.edges.end(), edge_matches),
                          workspace.edges.end());
    Commit(workspace);
}

void MissionWorkspaceService::AddEdge(const QString& from_node_id, const QString& to_node_id) {
    if (from_node_id.isEmpty() || to_node_id.isEmpty() || from_node_id == to_node_id) {
        return;
    }

    MissionWorkspace workspace = ActiveWorkspace();
    const auto same_edge = [&from_node_id, &to_node_id](const GraphEdge& edge) {
        return (edge.from_node_id == from_node_id && edge.to_node_id == to_node_id) ||
               (edge.from_node_id == to_node_id && edge.to_node_id == from_node_id);
    };
    if (std::any_of(workspace.edges.begin(), workspace.edges.end(), same_edge)) {
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
    workspace.edges.erase(std::remove_if(workspace.edges.begin(), workspace.edges.end(),
                                         [&edge_id](const GraphEdge& edge) {
                                             return edge.id == edge_id;
                                         }),
                          workspace.edges.end());
    Commit(workspace);
}

void MissionWorkspaceService::SetNodeType(const QString& node_id, GraphNodeType type) {
    MissionWorkspace workspace = ActiveWorkspace();
    for (GraphNode& node : workspace.nodes) {
        if (node.id == node_id) {
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
    const double x_step = static_cast<double>(image_size.width()) / static_cast<double>(columns - 1);
    const double y_step = static_cast<double>(image_size.height()) / static_cast<double>(rows - 1);

    QList<QList<QString>> node_ids;
    node_ids.resize(rows);
    for (int row = 0; row < rows; ++row) {
        node_ids[row].resize(columns);
        for (int column = 0; column < columns; ++column) {
            GraphNode node;
            node.id = NewId();
            node.label = QStringLiteral("N%1:%2").arg(row).arg(column);
            node.position = QPointF(x_step * column, y_step * row);
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
