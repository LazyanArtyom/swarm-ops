#pragma once

#include <QObject>
#include <QHash>
#include <QList>
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
    [[nodiscard]] QString WorkspaceFilePath() const;
    [[nodiscard]] QString WorkspaceDisplayName() const;
    [[nodiscard]] bool IsDirty() const;

    void NewWorkspace();
    bool LoadWorkspace(const QString& file_path, QString* error_message = nullptr);
    bool SaveWorkspace(QString* error_message = nullptr);
    bool SaveWorkspaceAs(const QString& file_path, QString* error_message = nullptr);
    void CreateWorkspaceFromImage(const QImage& image);
    void CreateWorkspaceFromMapCapture(const WorkspaceBackground& background);
    void AddNode(QPointF position);
    void MoveNode(const QString& node_id, QPointF position);
    void MoveNodes(const QHash<QString, QPointF>& node_positions);
    void RemoveNode(const QString& node_id);
    void RemoveItems(const QList<QString>& node_ids, const QList<QString>& edge_ids);
    void AddEdge(const QString& from_node_id, const QString& to_node_id);
    void RemoveEdge(const QString& edge_id);
    void SetNodeLabel(const QString& node_id, const QString& label);
    void SetNodeType(const QString& node_id, GraphNodeType type);
    void SetNodeCategory(const QString& node_id, GraphNodeCategory category);
    void GenerateGrid(int rows, int columns);
    void Undo();
    void Redo();
    [[nodiscard]] bool CanUndo() const;
    [[nodiscard]] bool CanRedo() const;
    void RequestGraphEditor();

   signals:
    void SigWorkspaceChanged(const app::mission::MissionWorkspace& workspace);
    void SigGraphEditorRequested();
    void SigDocumentStateChanged();
    void SigWorkspaceOperationRejected(const QString& message);

   private:
    [[nodiscard]] QString NextNodeLabel(const MissionWorkspace& workspace) const;
    [[nodiscard]] MissionWorkspace UntitledWorkspace() const;
    void Commit(MissionWorkspace workspace);
    void Restore(MissionWorkspace workspace);
    void SetDocumentState(QString file_path, bool dirty);
    void MarkDirty();

    std::unique_ptr<IMissionWorkspaceGateway> gateway_;
    QList<MissionWorkspace> undo_stack_;
    QList<MissionWorkspace> redo_stack_;
    QString workspace_file_path_;
    bool dirty_{false};
};

MissionWorkspaceService& MissionWorkspaceRuntime();

}  // namespace app::mission
