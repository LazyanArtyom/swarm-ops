#include "app/mission/ui/graph_editor_page.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QImageReader>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <utility>

#include "app/mission/mission_workspace_service.h"
#include "logging/logger.h"
#include "ui/theme/theme_metrics.h"

namespace app::mission {

namespace {

constexpr qreal kNodeRadius = 18.0;
constexpr qreal kMoveCommitEpsilon = 0.25;
constexpr auto kMissionLogCategory = "mission";

namespace icons {
constexpr auto kAddNode = ":/theme/icons/mission_add_node.svg";
constexpr auto kAttacker = ":/theme/icons/mission_attacker.png";
constexpr auto kBorder = ":/theme/icons/mission_border.png";
constexpr auto kCorner = ":/theme/icons/mission_corner.png";
constexpr auto kDeleteNode = ":/theme/icons/mission_delete_node.svg";
constexpr auto kDrone = ":/theme/icons/mission_drone.png";
constexpr auto kEdge = ":/theme/icons/mission_edge.svg";
constexpr auto kFit = ":/theme/icons/mission_fit.svg";
constexpr auto kGeneric = ":/theme/icons/mission_generic.png";
constexpr auto kGrid = ":/theme/icons/mission_grid.svg";
constexpr auto kPan = ":/theme/icons/mission_pan.svg";
constexpr auto kRedo = ":/theme/icons/mission_redo.svg";
constexpr auto kSelect = ":/theme/icons/mission_select.svg";
constexpr auto kTarget = ":/theme/icons/mission_target.svg";
constexpr auto kUndo = ":/theme/icons/mission_undo.svg";
constexpr auto kUploadImage = ":/theme/icons/mission_upload_image.svg";
constexpr auto kZoomIn = ":/theme/icons/mission_zoomin.svg";
constexpr auto kZoomOut = ":/theme/icons/mission_zoomout.svg";
}  // namespace icons

QRectF NodeBodyRect() {
    return QRectF(-kNodeRadius, -kNodeRadius, kNodeRadius * 2.0, kNodeRadius * 2.0);
}

bool HasMeaningfulMove(QPointF from, QPointF to) {
    const QPointF delta = to - from;
    return (delta.x() * delta.x() + delta.y() * delta.y()) > kMoveCommitEpsilon;
}

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
            return QString::fromLatin1(icons::kDrone);
        case GraphNodeCategory::kAttacker:
            return QString::fromLatin1(icons::kAttacker);
        case GraphNodeCategory::kTarget:
            return QString::fromLatin1(icons::kTarget);
        case GraphNodeCategory::kGeneric:
            return QString::fromLatin1(icons::kGeneric);
    }

    switch (node.type) {
        case GraphNodeType::kBorder:
            return QString::fromLatin1(icons::kBorder);
        case GraphNodeType::kCorner:
            return QString::fromLatin1(icons::kCorner);
    }

    return {};
}

QIcon EditorIcon(const char* path) {
    return QIcon(QString::fromLatin1(path));
}

QString GraphSummaryText(const MissionWorkspace& workspace) {
    return QStringLiteral("%1 nodes, %2 edges")
        .arg(workspace.nodes.size())
        .arg(workspace.edges.size());
}

QString BoundsSummaryText(const MapBounds& bounds) {
    return QStringLiteral("NW %1, %2\nSE %3, %4")
        .arg(bounds.north, 0, 'f', 6)
        .arg(bounds.west, 0, 'f', 6)
        .arg(bounds.south, 0, 'f', 6)
        .arg(bounds.east, 0, 'f', 6);
}

QIcon ToolModeIcon(GraphEditorToolMode mode) {
    switch (mode) {
        case GraphEditorToolMode::kAddNode:
            return EditorIcon(icons::kAddNode);
        case GraphEditorToolMode::kConnect:
            return EditorIcon(icons::kEdge);
        case GraphEditorToolMode::kPan:
            return EditorIcon(icons::kPan);
        case GraphEditorToolMode::kSelect:
            return EditorIcon(icons::kSelect);
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

const GraphNode* FindNodeById(const MissionWorkspace& workspace, const QString& node_id) {
    const auto it = std::find_if(workspace.nodes.begin(), workspace.nodes.end(),
                                 [&node_id](const GraphNode& node) { return node.id == node_id; });
    return it != workspace.nodes.end() ? &(*it) : nullptr;
}

const GraphEdge* FindEdgeById(const MissionWorkspace& workspace, const QString& edge_id) {
    const auto it = std::find_if(workspace.edges.begin(), workspace.edges.end(),
                                 [&edge_id](const GraphEdge& edge) { return edge.id == edge_id; });
    return it != workspace.edges.end() ? &(*it) : nullptr;
}

}  // namespace

GraphNodeItem::GraphNodeItem(GraphNode node, QGraphicsItem* parent)
    : QGraphicsObject(parent), node_(std::move(node)) {
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
    setPos(node_.position);
    setZValue(2.0);
}

QString GraphNodeItem::NodeId() const {
    return node_.id;
}

QRectF GraphNodeItem::boundingRect() const {
    return QRectF(-34.0, -34.0, 68.0, 76.0);
}

void GraphNodeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/,
                          QWidget* /*widget*/) {
    painter->setRenderHint(QPainter::Antialiasing);

    const QRectF body_rect = NodeBodyRect();
    if (hovered_ || isSelected() || connection_anchor_) {
        const QColor halo = connection_anchor_ ? QColor(0, 196, 255, 78)
                                               : (isSelected() ? QColor(66, 133, 244, 76)
                                                               : QColor(255, 255, 255, 42));
        painter->setPen(Qt::NoPen);
        painter->setBrush(halo);
        painter->drawEllipse(body_rect.adjusted(-8.0, -8.0, 8.0, 8.0));

        QPen ring(connection_anchor_ ? QColor(0, 214, 255, 235)
                                     : (isSelected() ? QColor(92, 170, 255, 235)
                                                     : QColor(255, 255, 255, 180)),
                  connection_anchor_ || isSelected() ? 2.6 : 1.8);
        ring.setCapStyle(Qt::RoundCap);
        if (connection_anchor_) {
            ring.setStyle(Qt::DashLine);
        }
        painter->setBrush(Qt::NoBrush);
        painter->setPen(ring);
        painter->drawEllipse(body_rect.adjusted(-4.0, -4.0, 4.0, 4.0));
    }

    const QString icon_path = NodeIconPath(node_);
    if (!icon_path.isEmpty()) {
        const QPixmap icon(icon_path);
        if (!icon.isNull()) {
            painter->setOpacity(isSelected() ? 0.9 : 1.0);
            painter->drawPixmap(body_rect.toRect(), icon);
            painter->setOpacity(1.0);
        }
    } else {
        painter->setPen(QPen(isSelected() ? QColor(255, 255, 255) : QColor(24, 28, 32), 2));
        painter->setBrush(NodeColor(node_));
        painter->drawEllipse(body_rect);
    }

    const QRectF label_rect(-kNodeRadius * 1.5, kNodeRadius + 2.0, kNodeRadius * 3.0, 16.0);
    painter->setPen(QColor(255, 255, 255, hovered_ || isSelected() ? 245 : 214));
    painter->drawText(label_rect, Qt::AlignCenter, node_.label);
}

QVariant GraphNodeItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemPositionHasChanged) {
        emit SigPositionChanged(node_.id, pos());
    }
    return QGraphicsObject::itemChange(change, value);
}

void GraphNodeItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    hovered_ = true;
    update();
    QGraphicsObject::hoverEnterEvent(event);
}

void GraphNodeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    hovered_ = false;
    update();
    QGraphicsObject::hoverLeaveEvent(event);
}

void GraphNodeItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    press_position_ = pos();
    setCursor(Qt::ClosedHandCursor);
    QGraphicsObject::mousePressEvent(event);
}

void GraphNodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsObject::mouseReleaseEvent(event);
    setCursor(Qt::OpenHandCursor);
    if (HasMeaningfulMove(press_position_, pos())) {
        emit SigMoveFinished(node_.id);
    }
}

void GraphNodeItem::SetConnectionAnchor(bool is_anchor) {
    if (connection_anchor_ == is_anchor) {
        return;
    }
    connection_anchor_ = is_anchor;
    update();
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
    UpdatePen();
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

QVariant GraphEdgeItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemSelectedHasChanged) {
        UpdatePen();
    }
    return QGraphicsLineItem::itemChange(change, value);
}

void GraphEdgeItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    hovered_ = true;
    UpdatePen();
    QGraphicsLineItem::hoverEnterEvent(event);
}

void GraphEdgeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    hovered_ = false;
    UpdatePen();
    QGraphicsLineItem::hoverLeaveEvent(event);
}

void GraphEdgeItem::UpdatePen() {
    QPen pen;
    if (isSelected()) {
        pen = QPen(QColor(0, 196, 255), 4.5);
    } else if (hovered_) {
        pen = QPen(QColor(255, 214, 105), 4.0);
    } else {
        pen = QPen(QColor(237, 91, 37), 3.0);
    }
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCosmetic(true);
    setPen(pen);
}

GraphEditorView::GraphEditorView(QWidget* parent) : QGraphicsView(parent) {
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform |
                   QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setCursor(Qt::ArrowCursor);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setFocusPolicy(Qt::StrongFocus);
    setFrameShape(QFrame::NoFrame);
    setAlignment(Qt::AlignCenter);
    setBackgroundBrush(QColor(18, 20, 24));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(scene_, &QGraphicsScene::selectionChanged, this, &GraphEditorView::HandleSelectionChanged);
}

void GraphEditorView::SetWorkspace(const MissionWorkspace& workspace) {
    const bool background_changed =
        workspace_.id != workspace.id ||
        workspace_.background.image.size() != workspace.background.image.size();
    workspace_ = workspace;
    auto_fit_pending_ = auto_fit_pending_ || background_changed;
    RebuildScene();
}

void GraphEditorView::SetToolMode(GraphEditorToolMode mode) {
    if (tool_mode_ == mode) {
        return;
    }

    tool_mode_ = mode;
    setDragMode(tool_mode_ == GraphEditorToolMode::kSelect ? QGraphicsView::RubberBandDrag
                                                           : QGraphicsView::NoDrag);
    if (tool_mode_ != GraphEditorToolMode::kConnect) {
        CancelPendingEdge();
    } else {
        UpdateViewCursor();
    }
}

GraphEditorToolMode GraphEditorView::ToolMode() const {
    return tool_mode_;
}

void GraphEditorView::OpenGridDialog() {
    ShowGridDialog();
}

void GraphEditorView::FitToWorkspace() {
    if (scene_ != nullptr && !scene_->sceneRect().isEmpty()) {
        resetTransform();
        fitInView(scene_->sceneRect(), Qt::KeepAspectRatioByExpanding);
        centerOn(scene_->sceneRect().center());
        auto_fit_pending_ = true;
        fitted_workspace_id_ = workspace_.id;
        fitted_background_size_ = workspace_.background.image.size();
    }
}

void GraphEditorView::ZoomIn() {
    auto_fit_pending_ = false;
    scale(1.2, 1.2);
}

void GraphEditorView::ZoomOut() {
    auto_fit_pending_ = false;
    scale(1.0 / 1.2, 1.0 / 1.2);
}

void GraphEditorView::contextMenuEvent(QContextMenuEvent* event) {
    GraphNodeItem* node_item = NodeItemAt(event->pos());
    GraphEdgeItem* edge_item = EdgeItemAt(event->pos());
    const bool clicked_selected_node = node_item != nullptr && node_item->isSelected();
    const bool clicked_selected_edge = edge_item != nullptr && edge_item->isSelected();
    const bool has_multi_selection = selected_node_ids_.size() + selected_edge_ids_.size() > 1;

    if (node_item != nullptr) {
        QMenu menu(this);
        QAction* set_edge = menu.addAction(EditorIcon(icons::kEdge), tr("Set Edge"));
        menu.addSeparator();
        QAction* set_drone = menu.addAction(EditorIcon(icons::kDrone), tr("Set Drone"));
        QAction* set_attacker = menu.addAction(EditorIcon(icons::kAttacker), tr("Set Attacker"));
        QAction* set_target = menu.addAction(EditorIcon(icons::kTarget), tr("Set Target"));
        QAction* set_generic = menu.addAction(EditorIcon(icons::kGeneric), tr("Set Generic"));
        menu.addSeparator();
        QAction* set_border = menu.addAction(EditorIcon(icons::kBorder), tr("Set Border"));
        QAction* set_corner = menu.addAction(EditorIcon(icons::kCorner), tr("Set Corner"));
        menu.addSeparator();
        QAction* delete_node =
            menu.addAction(EditorIcon(icons::kDeleteNode),
                           clicked_selected_node && has_multi_selection ? tr("Delete Selection")
                                                                        : tr("Delete"));
        QAction* action = menu.exec(event->globalPos());

        if (action == set_edge) {
            if (pending_edge_start_node_id_.isEmpty()) {
                StartPendingEdge(node_item);
            } else {
                const QString start_node_id = pending_edge_start_node_id_;
                const QString end_node_id = node_item->NodeId();
                CancelPendingEdge();
                MissionWorkspaceRuntime().AddEdge(start_node_id, end_node_id);
            }
            return;
        }
        if (action == delete_node) {
            if (clicked_selected_node) {
                DeleteSelection();
            } else {
                MissionWorkspaceRuntime().RemoveNode(node_item->NodeId());
            }
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
        QAction* delete_edge =
            menu.addAction(EditorIcon(icons::kDeleteNode),
                           clicked_selected_edge && has_multi_selection ? tr("Delete Selection")
                                                                        : tr("Delete"));
        if (menu.exec(event->globalPos()) == delete_edge) {
            if (clicked_selected_edge) {
                DeleteSelection();
            } else {
                MissionWorkspaceRuntime().RemoveEdge(edge_item->EdgeId());
            }
        }
        return;
    }

    QMenu menu(this);
    QAction* add_node = menu.addAction(EditorIcon(icons::kAddNode), tr("Add Node"));
    QAction* generate_grid = menu.addAction(EditorIcon(icons::kGrid), tr("Generate Grid"));
    menu.addSeparator();
    QAction* undo_action = menu.addAction(EditorIcon(icons::kUndo), tr("Undo"));
    undo_action->setEnabled(MissionWorkspaceRuntime().CanUndo());
    QAction* redo_action = menu.addAction(EditorIcon(icons::kRedo), tr("Redo"));
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
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        DeleteSelection();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        CancelPendingEdge();
        return;
    }
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        space_pressed_ = true;
        UpdateViewCursor();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

void GraphEditorView::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        space_pressed_ = false;
        UpdateViewCursor();
        return;
    }

    QGraphicsView::keyReleaseEvent(event);
}

void GraphEditorView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        if (GraphNodeItem* node_item = NodeItemAt(event->pos())) {
            if (node_item->isSelected()) {
                event->accept();
                return;
            }
            scene_->clearSelection();
            node_item->setSelected(true);
            event->accept();
            return;
        }
        if (GraphEdgeItem* edge_item = EdgeItemAt(event->pos())) {
            if (edge_item->isSelected()) {
                event->accept();
                return;
            }
            scene_->clearSelection();
            edge_item->setSelected(true);
            event->accept();
            return;
        }
    }

    if (IsPanGesture(event)) {
        panning_ = true;
        auto_fit_pending_ = false;
        last_pan_pos_ = event->pos();
        UpdateViewCursor();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (tool_mode_ == GraphEditorToolMode::kSelect &&
            event->modifiers().testFlag(Qt::ShiftModifier)) {
            if (GraphNodeItem* node_item = NodeItemAt(event->pos())) {
                node_item->setSelected(!node_item->isSelected());
                event->accept();
                return;
            }
            if (GraphEdgeItem* edge_item = EdgeItemAt(event->pos())) {
                edge_item->setSelected(!edge_item->isSelected());
                event->accept();
                return;
            }
        }

        if (tool_mode_ == GraphEditorToolMode::kAddNode && NodeItemAt(event->pos()) == nullptr &&
            EdgeItemAt(event->pos()) == nullptr) {
            MissionWorkspaceRuntime().AddNode(mapToScene(event->pos()));
            return;
        }

        if (tool_mode_ == GraphEditorToolMode::kConnect ||
            pending_edge_start_node_id_.isEmpty() == false) {
            if (GraphNodeItem* target_node = NodeItemAt(event->pos())) {
                if (pending_edge_start_node_id_.isEmpty()) {
                    StartPendingEdge(target_node);
                } else {
                    const QString start_node_id = pending_edge_start_node_id_;
                    const QString end_node_id = target_node->NodeId();
                    CancelPendingEdge();
                    MissionWorkspaceRuntime().AddEdge(start_node_id, end_node_id);
                }
                return;
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void GraphEditorView::mouseMoveEvent(QMouseEvent* event) {
    if (!pending_edge_start_node_id_.isEmpty()) {
        UpdatePendingEdgePreview(mapToScene(event->pos()));
    }

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
        UpdateViewCursor();
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void GraphEditorView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers().testFlag(Qt::ControlModifier) ||
        event->modifiers().testFlag(Qt::MetaModifier)) {
        if (event->angleDelta().y() > 0) {
            ZoomIn();
        } else {
            ZoomOut();
        }
        return;
    }

    QGraphicsView::wheelEvent(event);
}

void GraphEditorView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (auto_fit_pending_) {
        ScheduleFitToWorkspace();
    }
}

void GraphEditorView::showEvent(QShowEvent* event) {
    QGraphicsView::showEvent(event);
    if (auto_fit_pending_) {
        ScheduleFitToWorkspace();
    }
}

void GraphEditorView::RebuildScene() {
    if (scene_ == nullptr) {
        return;
    }

    pending_edge_preview_ = nullptr;
    const QSignalBlocker selection_blocker(scene_);
    scene_->clear();
    node_items_.clear();
    edge_items_.clear();
    if (tool_mode_ != GraphEditorToolMode::kConnect) {
        pending_edge_start_node_id_.clear();
    }
    UpdateViewCursor();

    if (!workspace_.background.IsValid()) {
        scene_->setSceneRect(QRectF(0, 0, 1000, 650));
        fitted_workspace_id_.clear();
        fitted_background_size_ = {};
        return;
    }

    auto* background_item = scene_->addPixmap(QPixmap::fromImage(workspace_.background.image));
    background_item->setTransformationMode(Qt::SmoothTransformation);
    background_item->setZValue(0.0);
    scene_->setSceneRect(QRectF(QPointF(0, 0), QSizeF(workspace_.background.image.size())));

    for (const GraphNode& node : workspace_.nodes) {
        auto* item = new GraphNodeItem(node);
        scene_->addItem(item);
        node_items_.insert(node.id, item);
        connect(item, &GraphNodeItem::SigMoveFinished, this,
                &GraphEditorView::CommitSelectedNodePositions);
    }

    for (const GraphEdge& edge : workspace_.edges) {
        GraphNodeItem* from = node_items_.value(edge.from_node_id, nullptr);
        GraphNodeItem* to = node_items_.value(edge.to_node_id, nullptr);
        if (from == nullptr || to == nullptr) {
            continue;
        }

        auto* edge_item = new GraphEdgeItem(edge, from, to);
        scene_->addItem(edge_item);
        edge_items_.insert(edge.id, edge_item);
    }

    RestoreSelection();

    if (auto_fit_pending_ &&
        (fitted_workspace_id_ != workspace_.id ||
         fitted_background_size_ != workspace_.background.image.size())) {
        ScheduleFitToWorkspace();
    }
}

void GraphEditorView::ScheduleFitToWorkspace() {
    QTimer::singleShot(0, this, [this] {
        if (auto_fit_pending_) {
            FitToWorkspace();
        }
    });
    QTimer::singleShot(60, this, [this] {
        if (auto_fit_pending_) {
            FitToWorkspace();
        }
    });
}

void GraphEditorView::RestoreSelection() {
    bool restored_selection = false;
    for (const QString& node_id : std::as_const(selected_node_ids_)) {
        if (GraphNodeItem* item = node_items_.value(node_id, nullptr)) {
            item->setSelected(true);
            restored_selection = true;
        }
    }
    for (const QString& edge_id : std::as_const(selected_edge_ids_)) {
        if (GraphEdgeItem* item = edge_items_.value(edge_id, nullptr)) {
            item->setSelected(true);
            restored_selection = true;
        }
    }
    if (!restored_selection) {
        selected_node_ids_.clear();
        selected_edge_ids_.clear();
    }
    HandleSelectionChanged();
}

void GraphEditorView::DeleteSelection() {
    if (selected_node_ids_.isEmpty() && selected_edge_ids_.isEmpty()) {
        return;
    }

    CancelPendingEdge();
    const QList<QString> node_ids = selected_node_ids_;
    const QList<QString> edge_ids = selected_edge_ids_;
    selected_node_ids_.clear();
    selected_edge_ids_.clear();
    MissionWorkspaceRuntime().RemoveItems(node_ids, edge_ids);
}

void GraphEditorView::CommitSelectedNodePositions() {
    if (scene_ == nullptr) {
        return;
    }

    QHash<QString, QPointF> node_positions;
    for (QGraphicsItem* item : scene_->selectedItems()) {
        if (auto* node_item = dynamic_cast<GraphNodeItem*>(item)) {
            node_positions.insert(node_item->NodeId(), node_item->pos());
        }
    }
    if (node_positions.isEmpty()) {
        return;
    }

    MissionWorkspaceRuntime().MoveNodes(node_positions);
}

void GraphEditorView::StartPendingEdge(GraphNodeItem* start_item) {
    if (start_item == nullptr || scene_ == nullptr) {
        return;
    }

    CancelPendingEdge();
    pending_edge_start_node_id_ = start_item->NodeId();
    start_item->SetConnectionAnchor(true);
    scene_->clearSelection();
    start_item->setSelected(true);

    pending_edge_preview_ = scene_->addLine(QLineF(start_item->pos(), start_item->pos()));
    QPen preview_pen(QColor(0, 196, 255, 220), 2.4, Qt::DashLine);
    preview_pen.setCapStyle(Qt::RoundCap);
    preview_pen.setCosmetic(true);
    pending_edge_preview_->setPen(preview_pen);
    pending_edge_preview_->setZValue(1.7);
    pending_edge_preview_->setAcceptedMouseButtons(Qt::NoButton);

    UpdateViewCursor();
}

void GraphEditorView::CancelPendingEdge() {
    if (GraphNodeItem* start_item = node_items_.value(pending_edge_start_node_id_, nullptr)) {
        start_item->SetConnectionAnchor(false);
    }
    if (pending_edge_preview_ != nullptr && scene_ != nullptr) {
        scene_->removeItem(pending_edge_preview_);
        delete pending_edge_preview_;
        pending_edge_preview_ = nullptr;
    }
    pending_edge_start_node_id_.clear();
    UpdateViewCursor();
}

void GraphEditorView::UpdatePendingEdgePreview(QPointF scene_pos) {
    if (pending_edge_preview_ == nullptr) {
        return;
    }
    GraphNodeItem* start_item = node_items_.value(pending_edge_start_node_id_, nullptr);
    if (start_item == nullptr) {
        CancelPendingEdge();
        return;
    }
    pending_edge_preview_->setLine(QLineF(start_item->pos(), scene_pos));
}

void GraphEditorView::UpdateViewCursor() {
    if (panning_) {
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (space_pressed_) {
        setCursor(Qt::OpenHandCursor);
        return;
    }
    if (tool_mode_ == GraphEditorToolMode::kPan) {
        setCursor(Qt::OpenHandCursor);
        return;
    }
    if (tool_mode_ == GraphEditorToolMode::kAddNode ||
        tool_mode_ == GraphEditorToolMode::kConnect ||
        pending_edge_start_node_id_.isEmpty() == false) {
        setCursor(Qt::CrossCursor);
        return;
    }
    setCursor(Qt::ArrowCursor);
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

void GraphEditorView::HandleSelectionChanged() {
    selected_node_ids_.clear();
    selected_edge_ids_.clear();

    const QList<QGraphicsItem*> selected_items = scene_ != nullptr ? scene_->selectedItems()
                                                                   : QList<QGraphicsItem*>{};
    for (QGraphicsItem* item : selected_items) {
        if (auto* node_item = dynamic_cast<GraphNodeItem*>(item)) {
            selected_node_ids_.push_back(node_item->NodeId());
        }
    }
    for (QGraphicsItem* item : selected_items) {
        if (auto* edge_item = dynamic_cast<GraphEdgeItem*>(item)) {
            selected_edge_ids_.push_back(edge_item->EdgeId());
        }
    }

    emit SigSelectionChanged(selected_node_ids_, selected_edge_ids_);
}

bool GraphEditorView::IsPanGesture(const QMouseEvent* event) const {
    return event != nullptr &&
           (event->button() == Qt::MiddleButton ||
            (event->button() == Qt::LeftButton && tool_mode_ == GraphEditorToolMode::kPan) ||
            (event->button() == Qt::LeftButton &&
             (event->modifiers().testFlag(Qt::ControlModifier) ||
              event->modifiers().testFlag(Qt::MetaModifier))) ||
            (event->button() == Qt::LeftButton && space_pressed_));
}

GraphInspectorPanel::GraphInspectorPanel(QWidget* parent) : QWidget(parent) {
    BuildUi();
    RefreshUi();
}

void GraphInspectorPanel::SetWorkspace(const MissionWorkspace& workspace) {
    workspace_ = workspace;
    RefreshUi();
}

void GraphInspectorPanel::SetSelection(const QList<QString>& node_ids, const QList<QString>& edge_ids) {
    if (selected_node_ids_ == node_ids && selected_edge_ids_ == edge_ids) {
        return;
    }
    selected_node_ids_ = node_ids;
    selected_edge_ids_ = edge_ids;
    RefreshUi();
}

void GraphInspectorPanel::BuildUi() {
    const auto metrics = ui::theme::ThemeMetrics::Instance().Current();

    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("uiComponent", QStringLiteral("panel-shell"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(metrics.spacing_md_px, metrics.spacing_md_px, metrics.spacing_md_px,
                               metrics.spacing_md_px);
    layout->setSpacing(metrics.spacing_md_px);

    auto* title = new QLabel(tr("Inspector"), this);
    title->setProperty("uiComponent", QStringLiteral("panel-title"));
    layout->addWidget(title);

    stack_ = new QStackedWidget(this);
    layout->addWidget(stack_, 1);

    auto* summary_page = new QWidget(stack_);
    auto* summary_layout = new QVBoxLayout(summary_page);
    summary_layout->setContentsMargins(0, 0, 0, 0);
    summary_layout->setSpacing(metrics.spacing_md_px);

    auto* summary_intro = new QLabel(tr("Select a node to edit mission graph properties."), summary_page);
    summary_intro->setWordWrap(true);
    summary_intro->setProperty("role", QStringLiteral("muted"));
    summary_layout->addWidget(summary_intro);

    auto* summary_form = new QFormLayout();
    summary_form->setContentsMargins(0, 0, 0, 0);
    summary_form->setSpacing(metrics.spacing_sm_px);
    workspace_name_value_ = new QLabel(summary_page);
    workspace_name_value_->setWordWrap(true);
    workspace_bounds_value_ = new QLabel(summary_page);
    workspace_bounds_value_->setWordWrap(true);
    workspace_bounds_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    workspace_graph_value_ = new QLabel(summary_page);
    workspace_graph_value_->setWordWrap(true);
    summary_form->addRow(tr("Workspace"), workspace_name_value_);
    summary_form->addRow(tr("Bounds"), workspace_bounds_value_);
    summary_form->addRow(tr("Graph"), workspace_graph_value_);
    summary_layout->addLayout(summary_form);
    summary_layout->addStretch(1);
    stack_->addWidget(summary_page);

    auto* node_page = new QWidget(stack_);
    auto* node_layout = new QVBoxLayout(node_page);
    node_layout->setContentsMargins(0, 0, 0, 0);
    node_layout->setSpacing(metrics.spacing_md_px);

    auto* node_title = new QLabel(tr("Node"), node_page);
    node_layout->addWidget(node_title);

    auto* node_form = new QFormLayout();
    node_form->setContentsMargins(0, 0, 0, 0);
    node_form->setSpacing(metrics.spacing_sm_px);

    node_id_value_ = new QLabel(node_page);
    node_id_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    node_label_edit_ = new QLineEdit(node_page);
    node_label_edit_->setProperty("settingsRole", QStringLiteral("field"));

    node_type_combo_ = new QComboBox(node_page);
    node_type_combo_->setProperty("settingsRole", QStringLiteral("field"));
    node_type_combo_->addItem(NodeTypeText(GraphNodeType::kBorder), static_cast<int>(GraphNodeType::kBorder));
    node_type_combo_->addItem(NodeTypeText(GraphNodeType::kCorner), static_cast<int>(GraphNodeType::kCorner));

    node_role_combo_ = new QComboBox(node_page);
    node_role_combo_->setProperty("settingsRole", QStringLiteral("field"));
    node_role_combo_->addItem(NodeCategoryText(GraphNodeCategory::kGeneric),
                              static_cast<int>(GraphNodeCategory::kGeneric));
    node_role_combo_->addItem(NodeCategoryText(GraphNodeCategory::kDrone),
                              static_cast<int>(GraphNodeCategory::kDrone));
    node_role_combo_->addItem(NodeCategoryText(GraphNodeCategory::kAttacker),
                              static_cast<int>(GraphNodeCategory::kAttacker));
    node_role_combo_->addItem(NodeCategoryText(GraphNodeCategory::kTarget),
                              static_cast<int>(GraphNodeCategory::kTarget));

    node_x_spin_ = new QDoubleSpinBox(node_page);
    node_x_spin_->setProperty("settingsRole", QStringLiteral("field"));
    node_x_spin_->setDecimals(1);
    node_x_spin_->setSingleStep(1.0);

    node_y_spin_ = new QDoubleSpinBox(node_page);
    node_y_spin_->setProperty("settingsRole", QStringLiteral("field"));
    node_y_spin_->setDecimals(1);
    node_y_spin_->setSingleStep(1.0);

    node_form->addRow(tr("Node ID"), node_id_value_);
    node_form->addRow(tr("Label"), node_label_edit_);
    node_form->addRow(tr("Type"), node_type_combo_);
    node_form->addRow(tr("Role"), node_role_combo_);
    node_form->addRow(tr("Position X"), node_x_spin_);
    node_form->addRow(tr("Position Y"), node_y_spin_);
    node_layout->addLayout(node_form);
    node_layout->addStretch(1);
    stack_->addWidget(node_page);

    auto* edge_page = new QWidget(stack_);
    auto* edge_layout = new QVBoxLayout(edge_page);
    edge_layout->setContentsMargins(0, 0, 0, 0);
    edge_layout->setSpacing(metrics.spacing_md_px);

    auto* edge_title = new QLabel(tr("Edge"), edge_page);
    edge_layout->addWidget(edge_title);

    auto* edge_form = new QFormLayout();
    edge_form->setContentsMargins(0, 0, 0, 0);
    edge_form->setSpacing(metrics.spacing_sm_px);
    edge_id_value_ = new QLabel(edge_page);
    edge_id_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    edge_from_value_ = new QLabel(edge_page);
    edge_from_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    edge_to_value_ = new QLabel(edge_page);
    edge_to_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    edge_form->addRow(tr("Edge ID"), edge_id_value_);
    edge_form->addRow(tr("From"), edge_from_value_);
    edge_form->addRow(tr("To"), edge_to_value_);
    edge_layout->addLayout(edge_form);
    edge_layout->addStretch(1);
    stack_->addWidget(edge_page);

    auto* multi_page = new QWidget(stack_);
    auto* multi_layout = new QVBoxLayout(multi_page);
    multi_layout->setContentsMargins(0, 0, 0, 0);
    multi_layout->setSpacing(metrics.spacing_md_px);

    auto* multi_title = new QLabel(tr("Selection"), multi_page);
    multi_layout->addWidget(multi_title);

    auto* multi_form = new QFormLayout();
    multi_form->setContentsMargins(0, 0, 0, 0);
    multi_form->setSpacing(metrics.spacing_sm_px);
    selection_count_value_ = new QLabel(multi_page);
    selection_breakdown_value_ = new QLabel(multi_page);
    selection_breakdown_value_->setWordWrap(true);
    multi_form->addRow(tr("Selected"), selection_count_value_);
    multi_form->addRow(tr("Items"), selection_breakdown_value_);
    multi_layout->addLayout(multi_form);
    multi_layout->addStretch(1);
    stack_->addWidget(multi_page);

    connect(node_label_edit_, &QLineEdit::editingFinished, this, [this] {
        if (syncing_ || selected_node_ids_.size() != 1) {
            return;
        }
        emit SigNodeLabelEdited(selected_node_ids_.front(), node_label_edit_->text());
    });

    connect(node_type_combo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (syncing_ || selected_node_ids_.size() != 1 || index < 0) {
                    return;
                }
                emit SigNodeTypeEdited(
                    selected_node_ids_.front(),
                    static_cast<GraphNodeType>(node_type_combo_->itemData(index).toInt()));
            });

    connect(node_role_combo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (syncing_ || selected_node_ids_.size() != 1 || index < 0) {
                    return;
                }
                emit SigNodeCategoryEdited(
                    selected_node_ids_.front(),
                    static_cast<GraphNodeCategory>(node_role_combo_->itemData(index).toInt()));
            });

    const auto emit_position_change = [this] {
        if (syncing_ || selected_node_ids_.size() != 1) {
            return;
        }
        emit SigNodePositionEdited(selected_node_ids_.front(),
                                   QPointF(node_x_spin_->value(), node_y_spin_->value()));
    };
    connect(node_x_spin_, &QDoubleSpinBox::editingFinished, this, emit_position_change);
    connect(node_y_spin_, &QDoubleSpinBox::editingFinished, this, emit_position_change);
}

void GraphInspectorPanel::RefreshUi() {
    RefreshSummary();
    if (stack_ == nullptr) {
        return;
    }
    if (selected_node_ids_.isEmpty() && selected_edge_ids_.isEmpty()) {
        stack_->setCurrentIndex(0);
        return;
    }
    if (selected_node_ids_.size() == 1 && selected_edge_ids_.isEmpty()) {
        RefreshNodeDetails();
        return;
    }
    if (selected_node_ids_.isEmpty() && selected_edge_ids_.size() == 1) {
        RefreshEdgeDetails();
        return;
    }
    RefreshMultiSelection();
}

void GraphInspectorPanel::RefreshSummary() {
    if (workspace_name_value_ != nullptr) {
        workspace_name_value_->setText(workspace_.name.isEmpty() ? tr("No active workspace")
                                                                 : workspace_.name);
    }
    if (workspace_bounds_value_ != nullptr) {
        workspace_bounds_value_->setText(workspace_.background.IsValid()
                                             ? BoundsSummaryText(workspace_.background.bounds)
                                             : tr("No map captured"));
    }
    if (workspace_graph_value_ != nullptr) {
        workspace_graph_value_->setText(GraphSummaryText(workspace_));
    }
}

void GraphInspectorPanel::RefreshNodeDetails() {
    const GraphNode* node = SelectedNode();
    if (node == nullptr) {
        stack_->setCurrentIndex(0);
        return;
    }

    stack_->setCurrentIndex(1);
    syncing_ = true;
    if (node_id_value_ != nullptr) {
        node_id_value_->setText(node->id);
    }
    if (node_label_edit_ != nullptr) {
        node_label_edit_->setText(node->label);
    }
    if (node_type_combo_ != nullptr) {
        node_type_combo_->setCurrentIndex(
            node_type_combo_->findData(static_cast<int>(node->type)));
    }
    if (node_role_combo_ != nullptr) {
        node_role_combo_->setCurrentIndex(
            node_role_combo_->findData(static_cast<int>(node->category)));
    }
    if (node_x_spin_ != nullptr) {
        node_x_spin_->setRange(0.0, std::max(1, workspace_.background.image.width()));
        node_x_spin_->setValue(node->position.x());
    }
    if (node_y_spin_ != nullptr) {
        node_y_spin_->setRange(0.0, std::max(1, workspace_.background.image.height()));
        node_y_spin_->setValue(node->position.y());
    }
    syncing_ = false;
}

void GraphInspectorPanel::RefreshEdgeDetails() {
    const GraphEdge* edge = SelectedEdge();
    if (edge == nullptr) {
        stack_->setCurrentIndex(0);
        return;
    }

    stack_->setCurrentIndex(2);
    if (edge_id_value_ != nullptr) {
        edge_id_value_->setText(edge->id);
    }
    if (edge_from_value_ != nullptr) {
        edge_from_value_->setText(edge->from_node_id);
    }
    if (edge_to_value_ != nullptr) {
        edge_to_value_->setText(edge->to_node_id);
    }
}

void GraphInspectorPanel::RefreshMultiSelection() {
    stack_->setCurrentIndex(3);
    const qsizetype total_count = selected_node_ids_.size() + selected_edge_ids_.size();
    if (selection_count_value_ != nullptr) {
        selection_count_value_->setText(QString::number(total_count));
    }
    if (selection_breakdown_value_ != nullptr) {
        selection_breakdown_value_->setText(
            tr("%1 nodes, %2 edges")
                .arg(selected_node_ids_.size())
                .arg(selected_edge_ids_.size()));
    }
}

const GraphNode* GraphInspectorPanel::SelectedNode() const {
    if (selected_node_ids_.size() != 1) {
        return nullptr;
    }
    return FindNodeById(workspace_, selected_node_ids_.front());
}

const GraphEdge* GraphInspectorPanel::SelectedEdge() const {
    if (selected_edge_ids_.size() != 1) {
        return nullptr;
    }
    return FindEdgeById(workspace_, selected_edge_ids_.front());
}

QString GraphInspectorPanel::NodeTypeText(GraphNodeType type) {
    switch (type) {
        case GraphNodeType::kBorder:
            return tr("Border");
        case GraphNodeType::kCorner:
            return tr("Corner");
    }
    return {};
}

QString GraphInspectorPanel::NodeCategoryText(GraphNodeCategory category) {
    switch (category) {
        case GraphNodeCategory::kGeneric:
            return tr("Generic");
        case GraphNodeCategory::kDrone:
            return tr("Drone");
        case GraphNodeCategory::kAttacker:
            return tr("Attacker");
        case GraphNodeCategory::kTarget:
            return tr("Target");
    }
    return {};
}

GraphEditorPage::GraphEditorPage(QWidget* parent) : QWidget(parent) {
    const auto metrics = ui::theme::ThemeMetrics::Instance().Current();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setContentsMargins(0, 0, 0, 0);

    tool_strip_ = new QToolBar(tr("Graph Editor"), this);
    tool_strip_->setProperty("uiComponent", QStringLiteral("toolbar-chrome"));
    tool_strip_->setMovable(false);
    tool_strip_->setFloatable(false);
    tool_strip_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tool_strip_->setIconSize(QSize(metrics.icon_md_px, metrics.icon_md_px));

    mode_actions_ = new QActionGroup(this);
    mode_actions_->setExclusive(true);

    select_action_ = tool_strip_->addAction(ToolModeIcon(GraphEditorToolMode::kSelect), tr("Select"));
    select_action_->setCheckable(true);
    select_action_->setChecked(true);
    mode_actions_->addAction(select_action_);

    pan_action_ = tool_strip_->addAction(ToolModeIcon(GraphEditorToolMode::kPan), tr("Pan"));
    pan_action_->setCheckable(true);
    mode_actions_->addAction(pan_action_);

    add_node_action_ = tool_strip_->addAction(EditorIcon(icons::kAddNode), tr("Add Node"));
    add_node_action_->setCheckable(true);
    mode_actions_->addAction(add_node_action_);

    connect_action_ = tool_strip_->addAction(EditorIcon(icons::kEdge), tr("Connect"));
    connect_action_->setCheckable(true);
    mode_actions_->addAction(connect_action_);

    tool_strip_->addSeparator();

    upload_image_action_ =
        tool_strip_->addAction(EditorIcon(icons::kUploadImage), tr("Upload Image"));
    fit_action_ = tool_strip_->addAction(EditorIcon(icons::kFit), tr("Fit"));
    zoom_in_action_ = tool_strip_->addAction(EditorIcon(icons::kZoomIn), tr("Zoom In"));
    zoom_out_action_ = tool_strip_->addAction(EditorIcon(icons::kZoomOut), tr("Zoom Out"));
    generate_grid_action_ = tool_strip_->addAction(EditorIcon(icons::kGrid), tr("Generate Grid"));

    tool_strip_->addSeparator();

    delete_action_ = tool_strip_->addAction(EditorIcon(icons::kDeleteNode), tr("Delete"));
    delete_action_->setEnabled(false);
    undo_action_ = tool_strip_->addAction(EditorIcon(icons::kUndo), tr("Undo"));
    redo_action_ = tool_strip_->addAction(EditorIcon(icons::kRedo), tr("Redo"));

    layout->addWidget(tool_strip_);

    auto* content_layout = new QHBoxLayout();
    content_layout->setContentsMargins(0, 0, 0, 0);
    content_layout->setSpacing(0);

    editor_ = new GraphEditorView(this);
    inspector_ = new GraphInspectorPanel(this);
    inspector_->setMinimumWidth(metrics.info_min_width_px);
    inspector_->setMaximumWidth(metrics.info_width_px);

    content_layout->addWidget(editor_, 1);
    content_layout->addWidget(inspector_);
    layout->addLayout(content_layout, 1);

    connect(select_action_, &QAction::triggered, this,
            [this] { editor_->SetToolMode(GraphEditorToolMode::kSelect); });
    connect(add_node_action_, &QAction::triggered, this,
            [this] { editor_->SetToolMode(GraphEditorToolMode::kAddNode); });
    connect(connect_action_, &QAction::triggered, this,
            [this] { editor_->SetToolMode(GraphEditorToolMode::kConnect); });
    connect(pan_action_, &QAction::triggered, this,
            [this] { editor_->SetToolMode(GraphEditorToolMode::kPan); });
    connect(upload_image_action_, &QAction::triggered, this, &GraphEditorPage::UploadImage);
    connect(fit_action_, &QAction::triggered, editor_, &GraphEditorView::FitToWorkspace);
    connect(zoom_in_action_, &QAction::triggered, editor_, &GraphEditorView::ZoomIn);
    connect(zoom_out_action_, &QAction::triggered, editor_, &GraphEditorView::ZoomOut);
    connect(generate_grid_action_, &QAction::triggered, editor_, &GraphEditorView::OpenGridDialog);
    connect(delete_action_, &QAction::triggered, editor_, &GraphEditorView::DeleteSelection);
    connect(undo_action_, &QAction::triggered, this, [] { MissionWorkspaceRuntime().Undo(); });
    connect(redo_action_, &QAction::triggered, this, [] { MissionWorkspaceRuntime().Redo(); });

    connect(editor_, &GraphEditorView::SigSelectionChanged, this, &GraphEditorPage::OnSelectionChanged);
    connect(inspector_, &GraphInspectorPanel::SigNodeLabelEdited, this,
            [](const QString& node_id, const QString& label) {
                MissionWorkspaceRuntime().SetNodeLabel(node_id, label);
            });
    connect(inspector_, &GraphInspectorPanel::SigNodeTypeEdited, this,
            [](const QString& node_id, GraphNodeType type) {
                MissionWorkspaceRuntime().SetNodeType(node_id, type);
            });
    connect(inspector_, &GraphInspectorPanel::SigNodeCategoryEdited, this,
            [](const QString& node_id, GraphNodeCategory category) {
                MissionWorkspaceRuntime().SetNodeCategory(node_id, category);
            });
    connect(inspector_, &GraphInspectorPanel::SigNodePositionEdited, this,
            [](const QString& node_id, QPointF position) {
                MissionWorkspaceRuntime().MoveNode(node_id, position);
            });

    connect(&MissionWorkspaceRuntime(), &MissionWorkspaceService::SigWorkspaceChanged, this,
            &GraphEditorPage::OnWorkspaceChanged);

    OnWorkspaceChanged(MissionWorkspaceRuntime().ActiveWorkspace());
}

void GraphEditorPage::OnSelectionChanged(const QList<QString>& node_ids,
                                         const QList<QString>& edge_ids) {
    if (inspector_ != nullptr) {
        inspector_->SetSelection(node_ids, edge_ids);
    }
    if (delete_action_ != nullptr) {
        delete_action_->setEnabled(!node_ids.isEmpty() || !edge_ids.isEmpty());
    }
}

void GraphEditorPage::OnWorkspaceChanged(const MissionWorkspace& workspace) {
    if (workspace.background.IsValid()) {
        logging::Logger::InfoFor(kMissionLogCategory, BoundsLogMessage(workspace));
    }
    if (editor_ != nullptr) {
        editor_->SetWorkspace(workspace);
    }
    if (inspector_ != nullptr) {
        inspector_->SetWorkspace(workspace);
    }
    RefreshToolbarState();
}

void GraphEditorPage::UploadImage() {
    QFileDialog dialog(this, tr("Upload Mission Image"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("Images (*.png *.jpg *.jpeg *.bmp *.webp *.tif *.tiff);;All Files (*)"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty()) {
        return;
    }

    const QString file_path = dialog.selectedFiles().front();
    QImageReader reader(file_path);
    reader.setAutoTransform(true);
    const QImage image = reader.read();
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Upload Image"),
                             tr("The selected image could not be loaded."));
        return;
    }

    MissionWorkspaceRuntime().CreateWorkspaceFromImage(image);
}

void GraphEditorPage::RefreshToolbarState() {
    if (editor_ == nullptr) {
        return;
    }

    switch (editor_->ToolMode()) {
        case GraphEditorToolMode::kSelect:
            select_action_->setChecked(true);
            break;
        case GraphEditorToolMode::kAddNode:
            add_node_action_->setChecked(true);
            break;
        case GraphEditorToolMode::kConnect:
            connect_action_->setChecked(true);
            break;
        case GraphEditorToolMode::kPan:
            pan_action_->setChecked(true);
            break;
    }

    if (undo_action_ != nullptr) {
        undo_action_->setEnabled(MissionWorkspaceRuntime().CanUndo());
    }
    if (redo_action_ != nullptr) {
        redo_action_->setEnabled(MissionWorkspaceRuntime().CanRedo());
    }
}

}  // namespace app::mission
