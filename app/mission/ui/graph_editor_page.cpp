#include "app/mission/ui/graph_editor_page.h"

#include <QContextMenuEvent>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>

#include "app/mission/mission_workspace_service.h"
#include "ui/theme/theme_metrics.h"

namespace app::mission {

namespace {

constexpr qreal kNodeRadius = 18.0;

QColor NodeColor(const GraphNode& node) {
    switch (node.category) {
        case GraphNodeCategory::kDrone:
            return QColor(66, 133, 244);
        case GraphNodeCategory::kTarget:
            return QColor(234, 67, 53);
        case GraphNodeCategory::kGeneric:
            break;
    }

    switch (node.type) {
        case GraphNodeType::kBorder:
            return QColor(251, 188, 5);
        case GraphNodeType::kCorner:
            return QColor(52, 168, 83);
        case GraphNodeType::kInner:
            return QColor(210, 216, 220);
    }

    return QColor(210, 216, 220);
}

}  // namespace

GraphNodeItem::GraphNodeItem(GraphNode node, QGraphicsItem* parent)
    : QGraphicsObject(parent), node_(std::move(node)) {
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
    setPos(node_.position);
    setZValue(2.0);
}

QString GraphNodeItem::NodeId() const {
    return node_.id;
}

QRectF GraphNodeItem::boundingRect() const {
    return QRectF(-kNodeRadius, -kNodeRadius, kNodeRadius * 2.0, kNodeRadius * 2.0);
}

void GraphNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/,
                          QWidget* /*widget*/) {
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(isSelected() ? QColor(255, 255, 255) : QColor(24, 28, 32), 2));
    painter->setBrush(NodeColor(node_));
    painter->drawEllipse(boundingRect());

    painter->setPen(QColor(20, 24, 28));
    painter->drawText(boundingRect(), Qt::AlignCenter, node_.label);
}

void GraphNodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsObject::mouseReleaseEvent(event);
    setCursor(Qt::OpenHandCursor);
    MissionWorkspaceRuntime().MoveNode(node_.id, pos());
}

GraphEdgeItem::GraphEdgeItem(GraphEdge edge, const QPointF& start, const QPointF& end,
                             QGraphicsItem* parent)
    : QGraphicsLineItem(QLineF(start, end), parent), edge_(std::move(edge)) {
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);
    setCursor(Qt::PointingHandCursor);
    setPen(QPen(QColor(237, 91, 37), 3));
    setZValue(1.0);
}

QString GraphEdgeItem::EdgeId() const {
    return edge_.id;
}

GraphEditorView::GraphEditorView(QWidget* parent) : QGraphicsView(parent) {
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform |
                   QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
}

void GraphEditorView::SetWorkspace(const MissionWorkspace& workspace) {
    workspace_ = workspace;
    RebuildScene();
}

void GraphEditorView::FitToWorkspace() {
    if (scene_ != nullptr && !scene_->sceneRect().isEmpty()) {
        fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    }
}

void GraphEditorView::contextMenuEvent(QContextMenuEvent* event) {
    GraphNodeItem* node_item = NodeItemAt(event->pos());
    GraphEdgeItem* edge_item = EdgeItemAt(event->pos());

    if (node_item != nullptr) {
        QMenu menu(this);
        QAction* set_edge = menu.addAction(tr("Set Edge"));
        QAction* delete_node = menu.addAction(tr("Delete"));
        QAction* action = menu.exec(event->globalPos());

        if (action == set_edge) {
            if (pending_edge_start_node_id_.isEmpty()) {
                pending_edge_start_node_id_ = node_item->NodeId();
            } else {
                MissionWorkspaceRuntime().AddEdge(pending_edge_start_node_id_, node_item->NodeId());
                pending_edge_start_node_id_.clear();
            }
            return;
        }
        if (action == delete_node) {
            MissionWorkspaceRuntime().RemoveNode(node_item->NodeId());
            return;
        }
    }

    if (edge_item != nullptr) {
        QMenu menu(this);
        QAction* delete_edge = menu.addAction(tr("Delete"));
        if (menu.exec(event->globalPos()) == delete_edge) {
            MissionWorkspaceRuntime().RemoveEdge(edge_item->EdgeId());
        }
        return;
    }

    QMenu menu(this);
    QAction* add_node = menu.addAction(tr("Add Node"));
    if (menu.exec(event->globalPos()) == add_node) {
        MissionWorkspaceRuntime().AddNode(mapToScene(event->pos()));
    }
}

void GraphEditorView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() == Qt::ControlModifier) {
        const qreal factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        scale(factor, factor);
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void GraphEditorView::RebuildScene() {
    if (scene_ == nullptr) {
        return;
    }

    scene_->clear();
    node_items_.clear();

    if (!workspace_.background.IsValid()) {
        scene_->setSceneRect(QRectF(0, 0, 1000, 650));
        return;
    }

    auto* background_item = scene_->addPixmap(QPixmap::fromImage(workspace_.background.image));
    background_item->setZValue(0.0);
    scene_->setSceneRect(QRectF(QPointF(0, 0), QSizeF(workspace_.background.image.size())));

    for (const GraphNode& node : workspace_.nodes) {
        auto* item = new GraphNodeItem(node);
        scene_->addItem(item);
        node_items_.insert(node.id, item);
    }

    for (const GraphEdge& edge : workspace_.edges) {
        GraphNodeItem* from = node_items_.value(edge.from_node_id, nullptr);
        GraphNodeItem* to = node_items_.value(edge.to_node_id, nullptr);
        if (from == nullptr || to == nullptr) {
            continue;
        }

        scene_->addItem(new GraphEdgeItem(edge, from->pos(), to->pos()));
    }
}

GraphNodeItem* GraphEditorView::NodeItemAt(const QPoint& view_pos) const {
    for (QGraphicsItem* item : items(view_pos)) {
        if (auto* node_item = dynamic_cast<GraphNodeItem*>(item)) {
            return node_item;
        }
    }
    return nullptr;
}

GraphEdgeItem* GraphEditorView::EdgeItemAt(const QPoint& view_pos) const {
    for (QGraphicsItem* item : items(view_pos)) {
        if (auto* edge_item = dynamic_cast<GraphEdgeItem*>(item)) {
            return edge_item;
        }
    }
    return nullptr;
}

GraphEditorPage::GraphEditorPage(QWidget* parent) : QWidget(parent) {
    const auto metrics = ui::theme::ThemeMetrics::Instance().Current();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(metrics.spacing_md_px, metrics.spacing_md_px, metrics.spacing_md_px,
                               metrics.spacing_md_px);
    layout->setSpacing(metrics.spacing_sm_px);

    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(0, 0, 0, 0);

    title_label_ = new QLabel(this);
    bounds_label_ = new QLabel(this);
    auto* grid_button = new QPushButton(tr("Grid"), this);
    auto* fit_button = new QPushButton(tr("Fit"), this);

    toolbar->addWidget(title_label_);
    toolbar->addWidget(bounds_label_);
    toolbar->addStretch();
    toolbar->addWidget(grid_button);
    toolbar->addWidget(fit_button);

    editor_ = new GraphEditorView(this);

    layout->addLayout(toolbar);
    layout->addWidget(editor_, 1);

    connect(&MissionWorkspaceRuntime(), &MissionWorkspaceService::SigWorkspaceChanged, this,
            &GraphEditorPage::OnWorkspaceChanged);
    connect(grid_button, &QPushButton::clicked, this,
            [] { MissionWorkspaceRuntime().GenerateGrid(5, 5); });
    connect(fit_button, &QPushButton::clicked, editor_, &GraphEditorView::FitToWorkspace);

    OnWorkspaceChanged(MissionWorkspaceRuntime().ActiveWorkspace());
}

void GraphEditorPage::OnWorkspaceChanged(const MissionWorkspace& workspace) {
    if (title_label_ != nullptr) {
        title_label_->setText(workspace.name.isEmpty() ? tr("Graph Editor") : workspace.name);
    }
    if (bounds_label_ != nullptr) {
        if (workspace.background.IsValid()) {
            const MapBounds& bounds = workspace.background.bounds;
            bounds_label_->setText(
                tr("NW %1, %2  SE %3, %4")
                    .arg(bounds.north, 0, 'f', 6)
                    .arg(bounds.west, 0, 'f', 6)
                    .arg(bounds.south, 0, 'f', 6)
                    .arg(bounds.east, 0, 'f', 6));
        } else {
            bounds_label_->clear();
        }
    }
    if (editor_ != nullptr) {
        editor_->SetWorkspace(workspace);
    }
}

}  // namespace app::mission
