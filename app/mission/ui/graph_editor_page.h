#pragma once

#include <QGraphicsObject>
#include <QGraphicsView>
#include <QHash>
#include <QPoint>
#include <QWidget>

#include "app/mission/mission_workspace.h"

class QGraphicsLineItem;
class QGraphicsPixmapItem;
class QLabel;

namespace app::mission {

class GraphNodeItem final : public QGraphicsObject {
    Q_OBJECT

   public:
    explicit GraphNodeItem(GraphNode node, QGraphicsItem* parent = nullptr);

    [[nodiscard]] QString NodeId() const;
    [[nodiscard]] QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

   signals:
    void SigPositionChanged(const QString& node_id, QPointF position);

   protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

   private:
    void SetConnectionAnchor(bool is_anchor);

    GraphNode node_;
    bool hovered_{false};
    bool connection_anchor_{false};

    friend class GraphEditorView;
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

    void SetWorkspace(const MissionWorkspace& workspace);
    void FitToWorkspace();

   protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

   private:
    void RebuildScene();
    void StartPendingEdge(GraphNodeItem* start_item);
    void CancelPendingEdge();
    void UpdatePendingEdgePreview(QPointF scene_pos);
    void ShowGridDialog();
    [[nodiscard]] GraphNodeItem* NodeItemAt(const QPoint& view_pos) const;
    [[nodiscard]] GraphEdgeItem* EdgeItemAt(const QPoint& view_pos) const;
    [[nodiscard]] bool IsPanGesture(const QMouseEvent* event) const;

    QGraphicsScene* scene_{nullptr};
    MissionWorkspace workspace_;
    QHash<QString, GraphNodeItem*> node_items_;
    QString pending_edge_start_node_id_;
    QGraphicsLineItem* pending_edge_preview_{nullptr};
    QPoint last_pan_pos_;
    bool panning_{false};
    bool space_pressed_{false};
    bool auto_fit_pending_{true};
};

class GraphEditorPage final : public QWidget {
    Q_OBJECT

   public:
    explicit GraphEditorPage(QWidget* parent = nullptr);

   private:
    void OnWorkspaceChanged(const MissionWorkspace& workspace);

    GraphEditorView* editor_{nullptr};
};

}  // namespace app::mission
