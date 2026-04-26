#pragma once

#include <QGraphicsObject>
#include <QGraphicsView>
#include <QHash>
#include <QList>
#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QWidget>

#include "app/mission/mission_workspace.h"
#include "ui/panels/panel_widget.h"

class QAction;
class QActionGroup;
class QComboBox;
class QGraphicsLineItem;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QSplitter;
class QStackedWidget;
class QToolBar;

namespace app::mission {

enum class GraphEditorToolMode {
    kSelect,
    kAddNode,
    kConnect,
    kPan,
};

class GraphNodeItem final : public QGraphicsObject {
    Q_OBJECT

   public:
    explicit GraphNodeItem(GraphNode node, QGraphicsItem* parent = nullptr);

    [[nodiscard]] QString NodeId() const;
    [[nodiscard]] QRectF boundingRect() const override;
    void SetConnectionAnchor(bool is_anchor);
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

   signals:
    void SigPositionChanged(const QString& node_id, QPointF position);
    void SigMoveFinished(const QString& node_id);

   protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

   private:
    GraphNode node_;
    QPointF press_position_;
    bool hovered_{false};
    bool connection_anchor_{false};
};

class GraphEdgeItem final : public QObject, public QGraphicsLineItem {
    Q_OBJECT

   public:
    GraphEdgeItem(GraphEdge edge, GraphNodeItem* start_item, GraphNodeItem* end_item,
                  QGraphicsItem* parent = nullptr);

    [[nodiscard]] QString EdgeId() const;

   public slots:
    void UpdatePosition();

   protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

   private:
    void UpdatePen();

    GraphEdge edge_;
    GraphNodeItem* start_item_{nullptr};
    GraphNodeItem* end_item_{nullptr};
    bool hovered_{false};
};

class GraphEditorView final : public QGraphicsView {
    Q_OBJECT

   public:
    explicit GraphEditorView(QWidget* parent = nullptr);
    ~GraphEditorView() override;

    void SetWorkspace(const MissionWorkspace& workspace);
    void SetToolMode(GraphEditorToolMode mode);
    [[nodiscard]] GraphEditorToolMode ToolMode() const;
    void FitToWorkspace();
    void ZoomIn();
    void ZoomOut();
    void OpenGridDialog();
    void DeleteSelection();

   signals:
    void SigSelectionChanged(const QList<QString>& node_ids, const QList<QString>& edge_ids);

   protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

   private:
    void HandleSelectionChanged();
    void RebuildScene();
    void RestoreSelection();
    void ScheduleFitToWorkspace();
    void CommitSelectedNodePositions();
    void StartPendingEdge(GraphNodeItem* start_item);
    void CancelPendingEdge();
    void UpdatePendingEdgePreview(QPointF scene_pos);
    void UpdateViewCursor();
    void ShowGridDialog();
    [[nodiscard]] GraphNodeItem* NodeItemAt(const QPoint& view_pos) const;
    [[nodiscard]] GraphEdgeItem* EdgeItemAt(const QPoint& view_pos) const;
    [[nodiscard]] bool IsPanGesture(const QMouseEvent* event) const;

    QGraphicsScene* scene_{nullptr};
    MissionWorkspace workspace_;
    QHash<QString, GraphNodeItem*> node_items_;
    QHash<QString, GraphEdgeItem*> edge_items_;
    QString pending_edge_start_node_id_;
    QGraphicsLineItem* pending_edge_preview_{nullptr};
    QList<QString> selected_node_ids_;
    QList<QString> selected_edge_ids_;
    QString fitted_workspace_id_;
    QSize fitted_background_size_;
    QPoint last_pan_pos_;
    GraphEditorToolMode tool_mode_{GraphEditorToolMode::kSelect};
    bool panning_{false};
    bool space_pressed_{false};
    bool auto_fit_pending_{true};
};

class GraphInspectorPanel final : public ui::PanelWidget {
    Q_OBJECT

   public:
    explicit GraphInspectorPanel(QWidget* parent = nullptr);

    void SetWorkspace(const MissionWorkspace& workspace);
    void SetSelection(const QList<QString>& node_ids, const QList<QString>& edge_ids);

   signals:
    void SigNodeLabelEdited(const QString& node_id, const QString& label);
    void SigNodeTypeEdited(const QString& node_id, GraphNodeType type);
    void SigNodeCategoryEdited(const QString& node_id, GraphNodeCategory category);
    void SigNodePositionEdited(const QString& node_id, QPointF position);

   private:
    void BuildUi();
    void RefreshUi();
    void RefreshSummary();
    void RefreshNodeDetails();
    void RefreshEdgeDetails();
    void RefreshMultiSelection();
    [[nodiscard]] const GraphNode* SelectedNode() const;
    [[nodiscard]] const GraphEdge* SelectedEdge() const;
    [[nodiscard]] static QString NodeTypeText(GraphNodeType type);
    [[nodiscard]] static QString NodeCategoryText(GraphNodeCategory category);

    MissionWorkspace workspace_;
    QList<QString> selected_node_ids_;
    QList<QString> selected_edge_ids_;
    QStackedWidget* stack_{nullptr};
    QWidget* content_host_{nullptr};
    QLabel* workspace_name_value_{nullptr};
    QLabel* workspace_bounds_value_{nullptr};
    QLabel* workspace_graph_value_{nullptr};
    QLabel* node_id_value_{nullptr};
    QLineEdit* node_label_edit_{nullptr};
    QComboBox* node_type_combo_{nullptr};
    QComboBox* node_role_combo_{nullptr};
    QDoubleSpinBox* node_x_spin_{nullptr};
    QDoubleSpinBox* node_y_spin_{nullptr};
    QLabel* edge_id_value_{nullptr};
    QLabel* edge_from_value_{nullptr};
    QLabel* edge_to_value_{nullptr};
    QLabel* selection_count_value_{nullptr};
    QLabel* selection_breakdown_value_{nullptr};
    bool syncing_{false};
};

class GraphEditorPage final : public QWidget {
    Q_OBJECT

   public:
    explicit GraphEditorPage(QWidget* parent = nullptr);

   private:
    void OnSelectionChanged(const QList<QString>& node_ids, const QList<QString>& edge_ids);
    void OnWorkspaceChanged(const MissionWorkspace& workspace);
    void UploadImage();
    void RefreshToolbarState();

    QToolBar* tool_strip_{nullptr};
    QSplitter* content_splitter_{nullptr};
    GraphEditorView* editor_{nullptr};
    GraphInspectorPanel* inspector_{nullptr};
    QActionGroup* mode_actions_{nullptr};
    QAction* select_action_{nullptr};
    QAction* add_node_action_{nullptr};
    QAction* connect_action_{nullptr};
    QAction* pan_action_{nullptr};
    QAction* upload_image_action_{nullptr};
    QAction* fit_action_{nullptr};
    QAction* zoom_in_action_{nullptr};
    QAction* zoom_out_action_{nullptr};
    QAction* generate_grid_action_{nullptr};
    QAction* delete_action_{nullptr};
    QAction* undo_action_{nullptr};
    QAction* redo_action_{nullptr};
};

}  // namespace app::mission
