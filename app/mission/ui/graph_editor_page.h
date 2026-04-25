#pragma once

#include <QGraphicsObject>
#include <QGraphicsView>
#include <QHash>
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

   protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

   private:
    GraphNode node_;
};

class GraphEdgeItem final : public QGraphicsLineItem {
   public:
    GraphEdgeItem(GraphEdge edge, const QPointF& start, const QPointF& end,
                  QGraphicsItem* parent = nullptr);

    [[nodiscard]] QString EdgeId() const;

   private:
    GraphEdge edge_;
};

class GraphEditorView final : public QGraphicsView {
    Q_OBJECT

   public:
    explicit GraphEditorView(QWidget* parent = nullptr);

    void SetWorkspace(const MissionWorkspace& workspace);
    void FitToWorkspace();

   protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

   private:
    void RebuildScene();
    [[nodiscard]] GraphNodeItem* NodeItemAt(const QPoint& view_pos) const;
    [[nodiscard]] GraphEdgeItem* EdgeItemAt(const QPoint& view_pos) const;

    QGraphicsScene* scene_{nullptr};
    MissionWorkspace workspace_;
    QHash<QString, GraphNodeItem*> node_items_;
    QString pending_edge_start_node_id_;
};

class GraphEditorPage final : public QWidget {
    Q_OBJECT

   public:
    explicit GraphEditorPage(QWidget* parent = nullptr);

   private:
    void OnWorkspaceChanged(const MissionWorkspace& workspace);

    QLabel* title_label_{nullptr};
    QLabel* bounds_label_{nullptr};
    GraphEditorView* editor_{nullptr};
};

}  // namespace app::mission
