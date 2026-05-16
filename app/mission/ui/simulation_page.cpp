#include "app/mission/ui/simulation_page.h"

#include <QAction>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointer>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSlider>
#include <QToolBar>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <array>
#include <cmath>

#include "app/client_gateway/client_gateway.h"
#include "app/client_gateway/swarm_runtime_client.h"
#include "app/mission/mission_workspace_service.h"
#include "app/mission/ui/workspace_view_fit.h"
#include "ui/theme/theme_icons.h"
#include "ui/theme/theme_metrics.h"

namespace app::mission {
namespace {

constexpr auto kDroneIcon = ":/theme/icons/mission_drone.png";
constexpr qreal kTrajectoryBaseOffsetPx = 8.0;
constexpr qreal kTrajectoryOffsetStepPx = 8.5;
constexpr qreal kTrajectoryLineWidthPx = 4.2;
constexpr qreal kTrajectoryOutlineWidthPx = 8.4;
constexpr qreal kVisibleLineMinLengthPx = 0.000001;
constexpr qreal kPathJoinEpsilonPx = 0.05;
constexpr int kTrajectoryAlpha = 255;
constexpr int kTrajectoryOutlineAlpha = 210;
constexpr int kSpeedSliderMin = 10;
constexpr int kSpeedSliderMax = 200;
constexpr int kSpeedSliderStep = 5;
constexpr int kSpeedSliderWidthPx = 160;
constexpr double kSpeedSliderScale = 100.0;

QColor TrajectoryColor(const QString& key) {
    static constexpr std::array<QRgb, 12> kDronePalette{
        qRgb(0, 216, 255),    qRgb(255, 57, 166),  qRgb(255, 214, 0),
        qRgb(31, 255, 113),   qRgb(255, 112, 31),  qRgb(168, 113, 255),
        qRgb(0, 245, 196),    qRgb(255, 53, 79),   qRgb(69, 142, 255),
        qRgb(255, 255, 255),  qRgb(191, 255, 0),   qRgb(255, 156, 226),
    };

    const int separator_index = key.lastIndexOf(QLatin1Char('-'));
    bool ok = false;
    const int drone_number =
        separator_index >= 0 ? key.mid(separator_index + 1).toInt(&ok) : 0;
    const qsizetype palette_index =
        ok && drone_number > 0
            ? static_cast<qsizetype>((drone_number - 1) % kDronePalette.size())
            : static_cast<qsizetype>(qHash(key) % kDronePalette.size());
    return QColor::fromRgb(kDronePalette[palette_index]);
}

QString SimulatedDroneId(int index) {
    return QStringLiteral("sim-drone-%1").arg(index + 1);
}

double DistanceBetween(QPointF lhs, QPointF rhs) {
    const QPointF delta = lhs - rhs;
    return std::hypot(delta.x(), delta.y());
}

bool IsMeaningfulSegment(QPointF from, QPointF to) {
    return DistanceBetween(from, to) > kVisibleLineMinLengthPx;
}

QString EdgeKey(QString from_node_id, QString to_node_id) {
    if (from_node_id.isEmpty() || to_node_id.isEmpty()) {
        return {};
    }
    return from_node_id < to_node_id ? QStringLiteral("%1:%2").arg(from_node_id, to_node_id)
                                     : QStringLiteral("%1:%2").arg(to_node_id, from_node_id);
}

QString TrajectoryKey(const QString& edge_key, const QString& drone_id, int edge_pass_index) {
    return QStringLiteral("%1|%2|%3").arg(edge_key).arg(drone_id).arg(edge_pass_index);
}

qreal TrajectoryLaneOffset(int lane) {
    if (lane <= 0) {
        return 0.0;
    }

    const int mirrored_lane = lane - 1;
    const qreal side = mirrored_lane % 2 == 0 ? 1.0 : -1.0;
    return side *
           (kTrajectoryBaseOffsetPx + static_cast<qreal>(mirrored_lane / 2) * kTrajectoryOffsetStepPx);
}

QLineF OffsetLine(QLineF line, QLineF reference_line, int lane) {
    const qreal offset_px = TrajectoryLaneOffset(lane);
    if (std::abs(offset_px) <= kVisibleLineMinLengthPx) {
        return line;
    }

    const QPointF delta = reference_line.p2() - reference_line.p1();
    const qreal length = std::hypot(delta.x(), delta.y());
    if (length <= kVisibleLineMinLengthPx) {
        return line;
    }

    const QPointF normal(-delta.y() / length, delta.x() / length);
    const QPointF offset(normal.x() * offset_px, normal.y() * offset_px);
    return QLineF(line.p1() + offset, line.p2() + offset);
}

void AppendTrajectoryPath(QGraphicsPathItem* item, const QLineF& line) {
    if (item == nullptr) {
        return;
    }

    QPainterPath path = item->path();
    if (path.isEmpty() ||
        DistanceBetween(path.currentPosition(), line.p1()) > kPathJoinEpsilonPx) {
        path.moveTo(line.p1());
    }
    path.lineTo(line.p2());
    item->setPath(path);
}

QPen TrajectoryPen(QColor color, qreal width) {
    QPen pen(color, width);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
}

QPen TrajectoryOutlinePen() {
    QColor outline(3, 7, 18);
    outline.setAlpha(kTrajectoryOutlineAlpha);
    return TrajectoryPen(outline, kTrajectoryOutlineWidthPx);
}

void DeletePathItems(QHash<QString, QGraphicsPathItem*>& items) {
    for (auto item = items.cbegin(); item != items.cend(); ++item) {
        delete item.value();
    }
    items.clear();
}

void DeleteDroneItems(QHash<QString, QGraphicsPixmapItem*>& items) {
    for (auto item = items.cbegin(); item != items.cend(); ++item) {
        delete item.value();
    }
    items.clear();
}

}  // namespace

SimulationView::SimulationView(QWidget* parent) : QGraphicsView(parent) {
    scene_ = new QGraphicsScene(this);
    setScene(scene_);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(Qt::NoBrush);
    setProperty("uiComponent", QStringLiteral("graph-editor-canvas"));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void SimulationView::SetWorkspace(const MissionWorkspace& workspace) {
    workspace_ = workspace;
    fit_to_workspace_active_ = true;
    RebuildScene();
}

void SimulationView::ApplyFrame(const client_gateway::MissionSimulationFrame& frame) {
    if (frame.workspace_id != workspace_.id) {
        return;
    }
    if (frame.state == client_gateway::MissionSimulationState::kIdle) {
        ResetSimulation();
        return;
    }

    for (const client_gateway::SimulatedDroneTrailSegment& segment : frame.trail_segments) {
        DrawTrajectorySegment(segment);
    }

    for (const client_gateway::SimulatedDronePosition& drone : frame.drones) {
        EnsureDroneItem(drone);
        auto* item = drone_items_.value(drone.drone_id, nullptr);
        if (item != nullptr) {
            item->setPos(drone.position);
        }
    }
}

void SimulationView::FitToWorkspace() {
    if (scene_ == nullptr || scene_->sceneRect().isEmpty()) {
        return;
    }
    fit_to_workspace_active_ = true;
    if (!isVisible()) {
        return;
    }
    (void)view_fit::ApplyExactSceneFit(*this, *scene_);
}

void SimulationView::ResetSimulation() {
    if (scene_ == nullptr) {
        return;
    }

    DeletePathItems(trajectory_outline_items_);
    DeletePathItems(trajectory_items_);
    trajectory_lanes_.clear();
    next_trajectory_lane_by_edge_.clear();
    DeleteDroneItems(drone_items_);
    RenderWorkspaceDrones();
}

void SimulationView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (fit_to_workspace_active_) {
        FitToWorkspace();
    }
}

void SimulationView::showEvent(QShowEvent* event) {
    QGraphicsView::showEvent(event);
    if (fit_to_workspace_active_) {
        FitToWorkspace();
    }
}

void SimulationView::RebuildScene() {
    scene_->clear();
    background_item_ = nullptr;
    node_positions_.clear();
    drone_items_.clear();
    trajectory_outline_items_.clear();
    trajectory_items_.clear();
    trajectory_lanes_.clear();
    next_trajectory_lane_by_edge_.clear();

    if (!workspace_.background.IsValid()) {
        scene_->setSceneRect(QRectF(0, 0, 1000, 650));
        return;
    }

    background_item_ = view_fit::AddWorkspaceBackground(*scene_, workspace_.background.image);
    for (const GraphNode& node : workspace_.nodes) {
        node_positions_.insert(node.id, node.position);
    }
    RenderWorkspaceDrones();
    ScheduleFitToWorkspace();
}

void SimulationView::ScheduleFitToWorkspace() {
    fit_to_workspace_active_ = true;

    const auto schedule_fit = [this](int delay_ms) {
        QTimer::singleShot(delay_ms, this, [guard = QPointer<SimulationView>(this)] {
            if (guard != nullptr && guard->fit_to_workspace_active_) {
                guard->FitToWorkspace();
            }
        });
    };

    schedule_fit(0);
    schedule_fit(50);
    schedule_fit(150);
}

void SimulationView::RenderWorkspaceDrones() {
    int drone_index = 0;
    for (const GraphNode& node : workspace_.nodes) {
        if (node.category != GraphNodeCategory::kDrone) {
            continue;
        }

        client_gateway::SimulatedDronePosition drone;
        drone.node_id = node.id;
        drone.drone_id = SimulatedDroneId(drone_index++);
        drone.position = node.position;
        drone.landed = false;
        EnsureDroneItem(drone);
    }
}

void SimulationView::EnsureDroneItem(const client_gateway::SimulatedDronePosition& drone) {
    if (drone_items_.contains(drone.drone_id)) {
        return;
    }

    QPixmap pixmap(QString::fromLatin1(kDroneIcon));
    if (pixmap.isNull()) {
        pixmap = QPixmap(28, 28);
        pixmap.fill(TrajectoryColor(drone.drone_id));
    }
    pixmap = pixmap.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    auto* item = scene_->addPixmap(pixmap);
    item->setOffset(-pixmap.width() / 2.0, -pixmap.height() / 2.0);
    item->setPos(drone.position);
    item->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    item->setZValue(2.0);
    drone_items_.insert(drone.drone_id, item);
}

void SimulationView::DrawTrajectorySegment(const client_gateway::SimulatedDroneTrailSegment& segment) {
    QString edge_key;
    QLineF edge_line;
    if (!TryEdgeLine(segment, &edge_key, &edge_line)) {
        return;
    }

    UpdateTrajectory(segment, edge_key, edge_line);
}

void SimulationView::UpdateTrajectory(
    const client_gateway::SimulatedDroneTrailSegment& segment, const QString& edge_key,
    const QLineF& edge_line) {
    const QString trajectory_key =
        TrajectoryKey(edge_key, segment.drone_id, segment.edge_pass_index);
    const int lane = TrajectoryLane(edge_key, segment.drone_id, segment.edge_pass_index);
    const QLineF visible_line =
        OffsetLine(QLineF(segment.start_position, segment.end_position), edge_line, lane);
    if (!IsMeaningfulSegment(visible_line.p1(), visible_line.p2())) {
        return;
    }

    QColor color = TrajectoryColor(segment.drone_id);
    color.setAlpha(kTrajectoryAlpha);
    QPen pen = TrajectoryPen(color, kTrajectoryLineWidthPx);

    auto* outline_item = trajectory_outline_items_.value(trajectory_key, nullptr);
    if (outline_item == nullptr) {
        outline_item = scene_->addPath(QPainterPath{}, TrajectoryOutlinePen());
        outline_item->setZValue(1.20 + static_cast<qreal>(lane) * 0.001);
        trajectory_outline_items_.insert(trajectory_key, outline_item);
    }

    auto* item = trajectory_items_.value(trajectory_key, nullptr);
    if (item == nullptr) {
        item = scene_->addPath(QPainterPath{}, pen);
        item->setZValue(1.45 + static_cast<qreal>(lane) * 0.001);
        trajectory_items_.insert(trajectory_key, item);
    }

    outline_item->setPen(TrajectoryOutlinePen());
    AppendTrajectoryPath(outline_item, visible_line);
    item->setPen(pen);
    AppendTrajectoryPath(item, visible_line);
}

bool SimulationView::TryEdgeLine(const client_gateway::SimulatedDroneTrailSegment& segment,
                                 QString* edge_key, QLineF* edge_line) const {
    const QString key = EdgeKey(segment.from_node_id, segment.to_node_id);
    if (key.isEmpty()) {
        return false;
    }

    const QString first_id =
        segment.from_node_id < segment.to_node_id ? segment.from_node_id : segment.to_node_id;
    const QString second_id =
        segment.from_node_id < segment.to_node_id ? segment.to_node_id : segment.from_node_id;
    const auto first = node_positions_.constFind(first_id);
    const auto second = node_positions_.constFind(second_id);
    if (first == node_positions_.constEnd() || second == node_positions_.constEnd()) {
        return false;
    }

    if (edge_key != nullptr) {
        *edge_key = key;
    }
    if (edge_line != nullptr) {
        *edge_line = QLineF(first.value(), second.value());
    }
    return true;
}

int SimulationView::TrajectoryLane(const QString& edge_key, const QString& drone_id,
                                   int edge_pass_index) {
    const QString trajectory_key = TrajectoryKey(edge_key, drone_id, edge_pass_index);
    const auto lane = trajectory_lanes_.constFind(trajectory_key);
    if (lane != trajectory_lanes_.constEnd()) {
        return lane.value();
    }

    const int next_lane = next_trajectory_lane_by_edge_.value(edge_key, 0);
    trajectory_lanes_.insert(trajectory_key, next_lane);
    next_trajectory_lane_by_edge_.insert(edge_key, next_lane + 1);
    return next_lane;
}

SimulationPage::SimulationPage(QWidget* parent) : QWidget(parent) {
    const auto metrics = ui::theme::ThemeMetrics::Instance().Current();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tool_strip_ = new QToolBar(tr("Mission Simulator"), this);
    tool_strip_->setProperty("uiComponent", QStringLiteral("toolbar-chrome"));
    tool_strip_->setMovable(false);
    tool_strip_->setFloatable(false);
    tool_strip_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tool_strip_->setIconSize(QSize(metrics.icon_md_px, metrics.icon_md_px));

    run_action_ = tool_strip_->addAction(tr("Start"));
    stop_action_ = tool_strip_->addAction(tr("Stop"));
    tool_strip_->addSeparator();

    speed_label_ = new QLabel(tool_strip_);
    speed_label_->setProperty("role", QStringLiteral("muted"));
    tool_strip_->addWidget(speed_label_);

    speed_slider_ = new QSlider(Qt::Horizontal, tool_strip_);
    speed_slider_->setRange(kSpeedSliderMin, kSpeedSliderMax);
    speed_slider_->setSingleStep(kSpeedSliderStep);
    speed_slider_->setPageStep(kSpeedSliderStep * 2);
    speed_slider_->setFixedWidth(kSpeedSliderWidthPx);
    speed_slider_->setValue(static_cast<int>(
        client_gateway::ClientGatewayRuntime().SwarmRuntime().MissionSimulationSpeedMultiplier() *
        kSpeedSliderScale));
    speed_slider_->setToolTip(tr("Mission simulation speed"));
    tool_strip_->addWidget(speed_slider_);

    tool_strip_->addSeparator();
    fit_action_ = tool_strip_->addAction(tr("Fit"));
    layout->addWidget(tool_strip_);

    view_ = new SimulationView(this);
    layout->addWidget(view_, 1);

    ui::theme::ThemeIcons::Instance().BindAction(run_action_, QStringLiteral("mission_sim_start"));
    ui::theme::ThemeIcons::Instance().BindAction(stop_action_, QStringLiteral("mission_sim_stop"));

    connect(run_action_, &QAction::triggered, this, &SimulationPage::ToggleRunState);
    connect(stop_action_, &QAction::triggered, this, &SimulationPage::StopSimulation);
    connect(speed_slider_, &QSlider::valueChanged, this, &SimulationPage::SetSimulationSpeed);
    connect(fit_action_, &QAction::triggered, view_, &SimulationView::FitToWorkspace);

    connect(&MissionWorkspaceRuntime(), &MissionWorkspaceService::SigWorkspaceChanged, this,
            &SimulationPage::OnWorkspaceChanged);
    connect(&client_gateway::ClientGatewayRuntime().SwarmRuntime(),
            &client_gateway::ISwarmRuntimeClient::SigMissionSimulationFrame, this,
            &SimulationPage::OnSimulationFrame);

    OnWorkspaceChanged(MissionWorkspaceRuntime().ActiveWorkspace());
    RefreshSpeedLabel();
    RefreshToolbarState(client_gateway::MissionSimulationState::kIdle);
}

void SimulationPage::ToggleRunState() {
    auto& runtime = client_gateway::ClientGatewayRuntime().SwarmRuntime();
    if (simulation_state_ == client_gateway::MissionSimulationState::kRunning) {
        runtime.PauseMissionSimulation();
        return;
    }
    if (simulation_state_ == client_gateway::MissionSimulationState::kPaused) {
        runtime.ResumeMissionSimulation();
        return;
    }

    StartSimulation();
}

void SimulationPage::StopSimulation() {
    client_gateway::ClientGatewayRuntime().SwarmRuntime().StopMissionSimulation();
    if (view_ != nullptr) {
        view_->ResetSimulation();
    }
    RefreshToolbarState(client_gateway::MissionSimulationState::kIdle);
}

void SimulationPage::StartSimulation() {
    if (view_ != nullptr) {
        view_->SetWorkspace(MissionWorkspaceRuntime().ActiveWorkspace());
    }
    const auto result = client_gateway::ClientGatewayRuntime().SwarmRuntime().StartMissionSimulation(
        MissionWorkspaceRuntime().ActiveWorkspace());
    if (!result.ok) {
        QMessageBox::information(this, tr("Mission Simulator"), result.message);
    }
}

void SimulationPage::SetSimulationSpeed(int slider_value) {
    client_gateway::ClientGatewayRuntime().SwarmRuntime().SetMissionSimulationSpeedMultiplier(
        static_cast<double>(slider_value) / kSpeedSliderScale);
    RefreshSpeedLabel();
}

void SimulationPage::OnWorkspaceChanged(const MissionWorkspace& workspace) {
    if (view_ != nullptr) {
        view_->SetWorkspace(workspace);
    }
}

void SimulationPage::OnSimulationFrame(const client_gateway::MissionSimulationFrame& frame) {
    if (view_ != nullptr) {
        view_->ApplyFrame(frame);
    }
    RefreshToolbarState(frame.state);
}

void SimulationPage::RefreshToolbarState(client_gateway::MissionSimulationState state) {
    simulation_state_ = state;
    const bool running = state == client_gateway::MissionSimulationState::kRunning;
    const bool paused = state == client_gateway::MissionSimulationState::kPaused;

    if (running) {
        run_action_->setText(tr("Pause"));
        ui::theme::ThemeIcons::Instance().BindAction(run_action_, QStringLiteral("mission_sim_pause"));
    } else if (paused) {
        run_action_->setText(tr("Resume"));
        ui::theme::ThemeIcons::Instance().BindAction(run_action_, QStringLiteral("mission_sim_start"));
    } else {
        run_action_->setText(tr("Start"));
        ui::theme::ThemeIcons::Instance().BindAction(run_action_, QStringLiteral("mission_sim_start"));
    }

    run_action_->setEnabled(true);
    stop_action_->setEnabled(running || paused);
}

void SimulationPage::RefreshSpeedLabel() {
    if (speed_label_ == nullptr) {
        return;
    }

    const double multiplier =
        client_gateway::ClientGatewayRuntime().SwarmRuntime().MissionSimulationSpeedMultiplier();
    speed_label_->setText(tr("Speed %1x").arg(multiplier, 0, 'f', 2));
}

}  // namespace app::mission
