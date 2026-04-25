#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "app/mission/mission_workspace.h"

namespace app::mission {

class IMissionWorkspaceGateway;

class MissionWorkspaceService final : public QObject {
    Q_OBJECT

   public:
    explicit MissionWorkspaceService(std::unique_ptr<IMissionWorkspaceGateway> gateway,
                                     QObject* parent = nullptr);

    [[nodiscard]] MissionWorkspace ActiveWorkspace() const;
    [[nodiscard]] bool HasActiveWorkspace() const;

    void CreateWorkspaceFromMapCapture(const WorkspaceBackground& background);
    void AddNode(QPointF position);
    void MoveNode(const QString& node_id, QPointF position);
    void RemoveNode(const QString& node_id);
    void AddEdge(const QString& from_node_id, const QString& to_node_id);
    void RemoveEdge(const QString& edge_id);
    void GenerateGrid(int rows, int columns);
    void RequestGraphEditor();

   signals:
    void SigWorkspaceChanged(const app::mission::MissionWorkspace& workspace);
    void SigGraphEditorRequested();

   private:
    [[nodiscard]] QString NextNodeLabel(const MissionWorkspace& workspace) const;
    void Commit(MissionWorkspace workspace);

    std::unique_ptr<IMissionWorkspaceGateway> gateway_;
};

MissionWorkspaceService& MissionWorkspaceRuntime();

}  // namespace app::mission
