#include "app/mission/ui/simulation_page.h"

#include <QAction>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
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
constexpr qreal kRepeatedTrajectoryBaseOffsetPx = 5.0;
constexpr qreal kRepeatedTrajectoryOffsetStepPx = 3.5;
constexpr qreal kPrimaryLineWidthPx = 3.2;
constexpr qreal kRepeatedPassLineWidthPx = 1.8;
constexpr qreal kVisibleLineMinLengthPx = 0.000001;
constexpr int kPrimaryPassAlpha = 235;
constexpr int kRepeatedPassAlpha = 245;
constexpr int kRepeatedPassMinValue = 92;
constexpr int kRepeatedPassBaseValueDrop = 82;
constexpr int kRepeatedPassValueDropStep = 18;
constexpr int kRepeatedPassMaxValueDrop = 150;
constexpr int kRepeatedPassBaseSaturationBoost = 42;
constexpr int kRepeatedPassSaturationBoostStep = 12;
constexpr int kRepeatedPassMaxSaturationBoost = 95;
constexpr int kSpeedSliderMin = 10;
constexpr int kSpeedSliderMax = 200;
constexpr int kSpeedSliderStep = 5;
constexpr int kSpeedSliderWidthPx = 160;
constexpr double kSpeedSliderScale = 100.0;

QColor TrajectoryColor(const QString& key) {
    static constexpr std::array<QRgb, 10> kDronePalette{
        qRgb(0, 213, 255),    qRgb(255, 79, 184),  qRgb(247, 208, 70),
        qRgb(99, 255, 106),   qRgb(255, 122, 47),  qRgb(155, 109, 255),
        qRgb(0, 224, 164),    qRgb(255, 92, 92),   qRgb(80, 145, 255),
        qRgb(245, 245, 245),
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

QString EdgeKey(QString from_node_id, QString to_node_id) {
    if (from_node_id.isEmpty() || to_node_id.isEmpty()) {
        return {};
    }
    return from_node_id < to_node_id ? QStringLiteral("%1:%2").arg(from_node_id, to_node_id)
                                     : QStringLiteral("%1:%2").arg(to_node_id, from_node_id);
}

QLineF OffsetLine(QLineF line, int lane) {
    if (lane < 0) {
        return line;
    }

    const QPointF delta = line.p2() - line.p1();
    const qreal length = std::hypot(delta.x(), delta.y());
    if (length <= kVisibleLineMinLengthPx) {
        return line;
    }

    const qreal side = lane % 2 == 0 ? 1.0 : -1.0;
    const qreal offset_px =
        side * (kRepeatedTrajectoryBaseOffsetPx +
                static_cast<qreal>(lane / 2) * kRepeatedTrajectoryOffsetStepPx);
    const QPointF normal(-delta.y() / length, delta.x() / length);
    const QPointF offset(normal.x() * offset_px, normal.y() * offset_px);
    return QLineF(line.p1() + offset, line.p2() + offset);
}

QPen TrajectoryPen(QColor color, qreal width) {
    QPen pen(color, width);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    return pen;
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
    primary_trajectory_items_.clear();
    secondary_trajectory_items_.clear();
    secondary_trajectory_lanes_.clear();
    next_secondary_lane_by_edge_.clear();

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

    UpdatePrimaryTrajectory(segment, edge_key, edge_line);
    if (segment.edge_pass_index > 1) {
        UpdateRepeatedTrajectory(segment, edge_key, edge_line);
    }
}

void SimulationView::UpdatePrimaryTrajectory(
    const client_gateway::SimulatedDroneTrailSegment& segment, const QString& edge_key,
    const QLineF& edge_line) {
    QColor color = TrajectoryColor(segment.drone_id);
    color.setAlpha(kPrimaryPassAlpha);
    QPen pen = TrajectoryPen(color, kPrimaryLineWidthPx);

    auto* item = primary_trajectory_items_.value(edge_key, nullptr);
    if (item == nullptr) {
        item = scene_->addLine(edge_line, pen);
        item->setZValue(1.38);
        primary_trajectory_items_.insert(edge_key, item);
        return;
    }
    item->setLine(edge_line);
    item->setPen(pen);
}

void SimulationView::UpdateRepeatedTrajectory(
    const client_gateway::SimulatedDroneTrailSegment& segment, const QString& edge_key,
    const QLineF& edge_line) {
    const QString secondary_key = QStringLiteral("%1|%2").arg(edge_key, segment.drone_id);
    const int lane = SecondaryTrajectoryLane(edge_key, segment.drone_id);
    const QLineF visible_line = OffsetLine(edge_line, lane);

    QPen pen = TrajectoryPen(TrajectorySegmentColor(segment), kRepeatedPassLineWidthPx);
    pen.setDashPattern({1.2, 3.6});

    auto* item = secondary_trajectory_items_.value(secondary_key, nullptr);
    if (item == nullptr) {
        item = scene_->addLine(visible_line, pen);
        item->setZValue(1.52 + static_cast<qreal>(lane) * 0.01);
        secondary_trajectory_items_.insert(secondary_key, item);
        return;
    }
    item->setLine(visible_line);
    item->setPen(pen);
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

QColor SimulationView::TrajectorySegmentColor(
    const client_gateway::SimulatedDroneTrailSegment& segment) const {
    QColor base = TrajectoryColor(segment.drone_id);
    if (segment.edge_pass_index <= 1) {
        base.setAlpha(kPrimaryPassAlpha);
        return base;
    }

    const int repeat_index = segment.edge_pass_index - 1;
    const int saturation =
        std::min(255, base.saturation() +
                          std::min(kRepeatedPassMaxSaturationBoost,
                                   kRepeatedPassBaseSaturationBoost +
                                       repeat_index * kRepeatedPassSaturationBoostStep));
    const int value =
        std::max(kRepeatedPassMinValue,
                 base.value() -
                     std::min(kRepeatedPassMaxValueDrop,
                              kRepeatedPassBaseValueDrop +
                                  repeat_index * kRepeatedPassValueDropStep));
    QColor repeated = QColor::fromHsv(base.hue(), saturation, value);
    repeated.setAlpha(kRepeatedPassAlpha);
    return repeated;
}

int SimulationView::SecondaryTrajectoryLane(const QString& edge_key, const QString& drone_id) {
    const QString secondary_key = QStringLiteral("%1|%2").arg(edge_key, drone_id);
    const auto lane = secondary_trajectory_lanes_.constFind(secondary_key);
    if (lane != secondary_trajectory_lanes_.constEnd()) {
        return lane.value();
    }

    const int next_lane = next_secondary_lane_by_edge_.value(edge_key, 0);
    secondary_trajectory_lanes_.insert(secondary_key, next_lane);
    next_secondary_lane_by_edge_.insert(edge_key, next_lane + 1);
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
