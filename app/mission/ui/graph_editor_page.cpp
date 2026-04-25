#include "app/mission/ui/graph_editor_page.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>

#include "app/mission/mission_workspace_service.h"
#include "logging/logger.h"

namespace app::mission {

namespace {

constexpr qreal kNodeRadius = 18.0;
constexpr auto kMissionLogCategory = "mission";

QColor NodeColor(const GraphNode& node) {
    switch (node.category) {
        case GraphNodeCategory::kDrone:
            return QColor(66, 133, 244);
        case GraphNodeCategory::kAttacker:
            return QColor(251, 188, 5);
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
    }

    return QColor(210, 216, 220);
}

QString NodeIconPath(const GraphNode& node) {
    switch (node.category) {
        case GraphNodeCategory::kDrone:
            return QStringLiteral(":/theme/icons/mission_drone.png");
        case GraphNodeCategory::kAttacker:
            return QStringLiteral(":/theme/icons/mission_attacker.png");
        case GraphNodeCategory::kTarget:
            return QStringLiteral(":/theme/icons/mission_target.svg");
        case GraphNodeCategory::kGeneric:
            return QStringLiteral(":/theme/icons/mission_generic.png");
    }

    switch (node.type) {
        case GraphNodeType::kBorder:
            return QStringLiteral(":/theme/icons/mission_border.png");
        case GraphNodeType::kCorner:
            return QStringLiteral(":/theme/icons/mission_corner.png");
    }

    return {};
}

std::string BoundsLogMessage(const MissionWorkspace& workspace) {
    const MapBounds& bounds = workspace.background.bounds;
    return QStringLiteral("Workspace %1 map bounds: NW(%2, %3), SE(%4, %5)")
        .arg(workspace.name)
        .arg(bounds.north, 0, 'f', 6)
        .arg(bounds.west, 0, 'f', 6)
        .arg(bounds.south, 0, 'f', 6)
        .arg(bounds.east, 0, 'f', 6)
        .toStdString();
}

}  // namespace

GraphNodeItem::GraphNodeItem(GraphNode node, QGraphicsItem* parent)
    : QGraphicsObject(parent), node_(std::move(node)) {
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
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

    const QString icon_path = NodeIconPath(node_);
    if (!icon_path.isEmpty()) {
        const QPixmap icon(icon_path);
        if (!icon.isNull()) {
            painter->setOpacity(isSelected() ? 0.72 : 1.0);
            painter->drawPixmap(boundingRect().toRect(), icon);
            painter->setOpacity(1.0);
        }
    } else {
        painter->setPen(QPen(isSelected() ? QColor(255, 255, 255) : QColor(24, 28, 32), 2));
        painter->setBrush(NodeColor(node_));
        painter->drawEllipse(boundingRect());
    }

    const QRectF label_rect(-kNodeRadius * 1.5, kNodeRadius + 2.0, kNodeRadius * 3.0, 16.0);
    painter->setPen(QColor(255, 255, 255));
    painter->drawText(label_rect, Qt::AlignCenter, node_.label);
}

QVariant GraphNodeItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemPositionHasChanged) {
        emit SigPositionChanged(node_.id, pos());
    }
    return QGraphicsObject::itemChange(change, value);
}

void GraphNodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsObject::mouseReleaseEvent(event);
    setCursor(Qt::OpenHandCursor);
    MissionWorkspaceRuntime().MoveNode(node_.id, pos());
}

GraphEdgeItem::GraphEdgeItem(GraphEdge edge, GraphNodeItem* start_item, GraphNodeItem* end_item,
                             QGraphicsItem* parent)
    : QGraphicsLineItem(parent),
      edge_(std::move(edge)),
      start_item_(start_item),
      end_item_(end_item) {
    setFlag(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);
    setCursor(Qt::PointingHandCursor);
    setPen(QPen(QColor(237, 91, 37), 3));
    setZValue(1.0);

    if (start_item_ != nullptr) {
        connect(start_item_, &GraphNodeItem::SigPositionChanged, this, &GraphEdgeItem::UpdatePosition);
    }
    if (end_item_ != nullptr) {
        connect(end_item_, &GraphNodeItem::SigPositionChanged, this, &GraphEdgeItem::UpdatePosition);
    }
    UpdatePosition();
}

QString GraphEdgeItem::EdgeId() const {
    return edge_.id;
}

void GraphEdgeItem::UpdatePosition() {
    if (start_item_ == nullptr || end_item_ == nullptr) {
        return;
    }
    setLine(QLineF(start_item_->pos(), end_item_->pos()));
}

GraphEditorView::GraphEditorView(QWidget* parent) : QGraphicsView(parent) {
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform |
                   QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setCursor(Qt::ArrowCursor);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setFocusPolicy(Qt::StrongFocus);
}

void GraphEditorView::SetWorkspace(const MissionWorkspace& workspace) {
    workspace_ = workspace;
    auto_fit_pending_ = true;
    RebuildScene();
}

void GraphEditorView::FitToWorkspace() {
    if (scene_ != nullptr && !scene_->sceneRect().isEmpty()) {
        fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
        auto_fit_pending_ = true;
    }
}

void GraphEditorView::contextMenuEvent(QContextMenuEvent* event) {
    GraphNodeItem* node_item = NodeItemAt(event->pos());
    GraphEdgeItem* edge_item = EdgeItemAt(event->pos());

    if (node_item != nullptr) {
        QMenu menu(this);
        QAction* set_edge =
            menu.addAction(QIcon(QStringLiteral(":/theme/icons/mission_edge.svg")), tr("Set Edge"));
        menu.addSeparator();
        QAction* set_drone =
            menu.addAction(QIcon(QStringLiteral(":/theme/icons/mission_drone.png")), tr("Set Drone"));
        QAction* set_attacker = menu.addAction(
            QIcon(QStringLiteral(":/theme/icons/mission_attacker.png")), tr("Set Attacker"));
        QAction* set_target =
            menu.addAction(QIcon(QStringLiteral(":/theme/icons/mission_target.svg")), tr("Set Target"));
        QAction* set_generic = menu.addAction(
            QIcon(QStringLiteral(":/theme/icons/mission_generic.png")), tr("Set Generic"));
        menu.addSeparator();
        QAction* set_border =
            menu.addAction(QIcon(QStringLiteral(":/theme/icons/mission_border.png")), tr("Set Border"));
        QAction* set_corner =
            menu.addAction(QIcon(QStringLiteral(":/theme/icons/mission_corner.png")), tr("Set Corner"));
        menu.addSeparator();
        QAction* delete_node = menu.addAction(
            QIcon(QStringLiteral(":/theme/icons/mission_delete_node.svg")), tr("Delete"));
        QAction* action = menu.exec(event->globalPos());

        if (action == set_edge) {
            if (pending_edge_start_node_id_.isEmpty()) {
                pending_edge_start_node_id_ = node_item->NodeId();
                setCursor(Qt::CrossCursor);
            } else {
                MissionWorkspaceRuntime().AddEdge(pending_edge_start_node_id_, node_item->NodeId());
                pending_edge_start_node_id_.clear();
                setCursor(Qt::ArrowCursor);
            }
            return;
        }
        if (action == delete_node) {
            MissionWorkspaceRuntime().RemoveNode(node_item->NodeId());
            return;
        }
        if (action == set_drone) {
            MissionWorkspaceRuntime().SetNodeCategory(node_item->NodeId(), GraphNodeCategory::kDrone);
            return;
        }
        if (action == set_attacker) {
            MissionWorkspaceRuntime().SetNodeCategory(node_item->NodeId(), GraphNodeCategory::kAttacker);
            return;
        }
        if (action == set_target) {
            MissionWorkspaceRuntime().SetNodeCategory(node_item->NodeId(), GraphNodeCategory::kTarget);
            return;
        }
        if (action == set_generic) {
            MissionWorkspaceRuntime().SetNodeCategory(node_item->NodeId(), GraphNodeCategory::kGeneric);
            return;
        }
        if (action == set_border) {
            MissionWorkspaceRuntime().SetNodeType(node_item->NodeId(), GraphNodeType::kBorder);
            return;
        }
        if (action == set_corner) {
            MissionWorkspaceRuntime().SetNodeType(node_item->NodeId(), GraphNodeType::kCorner);
            return;
        }
    }

    if (edge_item != nullptr) {
        QMenu menu(this);
        QAction* delete_edge = menu.addAction(
            QIcon(QStringLiteral(":/theme/icons/mission_delete_node.svg")), tr("Delete"));
        if (menu.exec(event->globalPos()) == delete_edge) {
            MissionWorkspaceRuntime().RemoveEdge(edge_item->EdgeId());
        }
        return;
    }

    QMenu menu(this);
    QAction* add_node = menu.addAction(tr("Add Node"));
    QAction* generate_grid =
        menu.addAction(QIcon(QStringLiteral(":/theme/icons/mission_grid.svg")), tr("Generate Grid"));
    menu.addSeparator();
    QAction* undo_action = menu.addAction(tr("Undo"));
    undo_action->setEnabled(MissionWorkspaceRuntime().CanUndo());
    QAction* redo_action = menu.addAction(tr("Redo"));
    redo_action->setEnabled(MissionWorkspaceRuntime().CanRedo());

    QAction* action = menu.exec(event->globalPos());
    if (action == add_node) {
        MissionWorkspaceRuntime().AddNode(mapToScene(event->pos()));
    } else if (action == generate_grid) {
        ShowGridDialog();
    } else if (action == undo_action) {
        MissionWorkspaceRuntime().Undo();
    } else if (action == redo_action) {
        MissionWorkspaceRuntime().Redo();
    }
}

void GraphEditorView::keyPressEvent(QKeyEvent* event) {
    if (event->matches(QKeySequence::Undo)) {
        MissionWorkspaceRuntime().Undo();
        return;
    }
    if (event->matches(QKeySequence::Redo)) {
        MissionWorkspaceRuntime().Redo();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        CancelPendingEdge();
        return;
    }
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        space_pressed_ = true;
        setCursor(Qt::OpenHandCursor);
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void GraphEditorView::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        space_pressed_ = false;
        if (!panning_) {
            setCursor(pending_edge_start_node_id_.isEmpty() ? Qt::ArrowCursor : Qt::CrossCursor);
        }
        return;
    }

    QGraphicsView::keyReleaseEvent(event);
}

void GraphEditorView::mousePressEvent(QMouseEvent* event) {
    if (pending_edge_start_node_id_.isEmpty() == false && event->button() == Qt::LeftButton) {
        if (GraphNodeItem* target_node = NodeItemAt(event->pos())) {
            MissionWorkspaceRuntime().AddEdge(pending_edge_start_node_id_, target_node->NodeId());
            CancelPendingEdge();
            return;
        }
    }

    if (IsPanGesture(event)) {
        panning_ = true;
        auto_fit_pending_ = false;
        last_pan_pos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void GraphEditorView::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        const QPoint delta = event->pos() - last_pan_pos_;
        last_pan_pos_ = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void GraphEditorView::mouseReleaseEvent(QMouseEvent* event) {
    if (panning_) {
        panning_ = false;
        setCursor(space_pressed_ ? Qt::OpenHandCursor
                                 : (pending_edge_start_node_id_.isEmpty() ? Qt::ArrowCursor
                                                                          : Qt::CrossCursor));
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GraphEditorView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() == Qt::ControlModifier) {
        const qreal factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        auto_fit_pending_ = false;
        scale(factor, factor);
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void GraphEditorView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (auto_fit_pending_) {
        FitToWorkspace();
    }
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

        scene_->addItem(new GraphEdgeItem(edge, from, to));
    }

    if (auto_fit_pending_) {
        FitToWorkspace();
    }
}

void GraphEditorView::CancelPendingEdge() {
    pending_edge_start_node_id_.clear();
    if (!space_pressed_ && !panning_) {
        setCursor(Qt::ArrowCursor);
    }
}

void GraphEditorView::ShowGridDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Generate Grid"));

    auto* layout = new QVBoxLayout(&dialog);
    auto* rows = new QSpinBox(&dialog);
    auto* columns = new QSpinBox(&dialog);
    rows->setRange(2, 100);
    columns->setRange(2, 100);
    rows->setValue(5);
    columns->setValue(5);
    rows->setPrefix(tr("Rows: "));
    columns->setPrefix(tr("Columns: "));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(rows);
    layout->addWidget(columns);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        MissionWorkspaceRuntime().GenerateGrid(rows->value(), columns->value());
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

bool GraphEditorView::IsPanGesture(const QMouseEvent* event) const {
    return event != nullptr &&
           (event->button() == Qt::MiddleButton ||
            (event->button() == Qt::LeftButton && space_pressed_));
}

GraphEditorPage::GraphEditorPage(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setContentsMargins(0, 0, 0, 0);

    editor_ = new GraphEditorView(this);

    layout->addWidget(editor_, 1);

    connect(&MissionWorkspaceRuntime(), &MissionWorkspaceService::SigWorkspaceChanged, this,
            &GraphEditorPage::OnWorkspaceChanged);

    OnWorkspaceChanged(MissionWorkspaceRuntime().ActiveWorkspace());
}

void GraphEditorPage::OnWorkspaceChanged(const MissionWorkspace& workspace) {
    if (workspace.background.IsValid()) {
        logging::Logger::InfoFor(kMissionLogCategory, BoundsLogMessage(workspace));
    }
    if (editor_ != nullptr) {
        editor_->SetWorkspace(workspace);
    }
}

}  // namespace app::mission
