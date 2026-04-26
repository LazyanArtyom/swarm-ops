#include "app/mission/ui/simulation_page.h"

#include <QAction>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QShowEvent>
#include <QToolBar>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <array>
#include <cmath>

#include "app/client_gateway/client_gateway.h"
#include "app/client_gateway/swarm_runtime_client.h"
#include "app/mission/mission_workspace_service.h"
#include "ui/theme/theme_metrics.h"

namespace app::mission {
namespace {

constexpr auto kDroneIcon = ":/theme/icons/mission_drone.png";

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

QLineF VisibleTrajectoryLine(const client_gateway::SimulatedDroneTrailSegment& segment) {
    QLineF line(segment.start_position, segment.end_position);
    if (segment.edge_pass_index <= 1) {
        return line;
    }

    const QPointF delta = segment.end_position - segment.start_position;
    const qreal length = std::hypot(delta.x(), delta.y());
    if (length <= 0.000001) {
        return line;
    }

    const int repeat_index = segment.edge_pass_index - 1;
    const qreal side = repeat_index % 2 == 1 ? 1.0 : -1.0;
    const qreal offset_px = side * (4.0 + static_cast<qreal>((repeat_index - 1) / 2) * 3.0);
    const QPointF normal(-delta.y() / length, delta.x() / length);
    const QPointF offset(normal.x() * offset_px, normal.y() * offset_px);
    return QLineF(segment.start_position + offset, segment.end_position + offset);
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
    if (!isVisible() || viewport()->size().width() <= 1 || viewport()->size().height() <= 1) {
        fit_pending_ = true;
        return;
    }
    resetTransform();
    fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    centerOn(scene_->sceneRect().center());
    fit_pending_ = false;
}

void SimulationView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    if (fit_pending_) {
        FitToWorkspace();
    }
}

void SimulationView::showEvent(QShowEvent* event) {
    QGraphicsView::showEvent(event);
    if (fit_pending_) {
        FitToWorkspace();
    }
}

void SimulationView::RebuildScene() {
    scene_->clear();
    background_item_ = nullptr;
    drone_items_.clear();

    if (!workspace_.background.IsValid()) {
        scene_->setSceneRect(QRectF(0, 0, 1000, 650));
        fit_pending_ = true;
        return;
    }

    background_item_ = scene_->addPixmap(QPixmap::fromImage(workspace_.background.image));
    background_item_->setTransformationMode(Qt::SmoothTransformation);
    background_item_->setZValue(0.0);
    scene_->setSceneRect(QRectF(QPointF(0, 0), QSizeF(workspace_.background.image.size())));
    RenderWorkspaceDrones();
    fit_pending_ = true;
    ScheduleFitToWorkspace();
}

void SimulationView::ScheduleFitToWorkspace() {
    QTimer::singleShot(0, this, [this] {
        if (fit_pending_) {
            FitToWorkspace();
        }
    });
    QTimer::singleShot(60, this, [this] {
        if (fit_pending_) {
            FitToWorkspace();
        }
    });
}

void SimulationView::RenderWorkspaceDrones() {
    int drone_index = 1;
    for (const GraphNode& node : workspace_.nodes) {
        if (node.category != GraphNodeCategory::kDrone) {
            continue;
        }

        client_gateway::SimulatedDronePosition drone;
        drone.node_id = node.id;
        drone.drone_id = QStringLiteral("sim-drone-%1").arg(drone_index++);
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
    item->setZValue(2.0);
    drone_items_.insert(drone.drone_id, item);
}

void SimulationView::DrawTrajectorySegment(const client_gateway::SimulatedDroneTrailSegment& segment) {
    const QPointF delta = segment.end_position - segment.start_position;
    if ((delta.x() * delta.x() + delta.y() * delta.y()) <= 0.01) {
        return;
    }

    const QLineF visible_line = VisibleTrajectoryLine(segment);
    const bool repeated_pass = segment.edge_pass_index > 1;

    QColor shadow_color(4, 8, 12, repeated_pass ? 175 : 120);
    auto* shadow_item =
        scene_->addLine(visible_line, TrajectoryPen(shadow_color, repeated_pass ? 3.8 : 4.6));
    shadow_item->setZValue(repeated_pass ? 1.48 : 1.36);

    QPen color_pen = TrajectoryPen(TrajectorySegmentColor(segment), repeated_pass ? 2.1 : 2.8);
    if (repeated_pass) {
        color_pen.setDashPattern({1.4, 3.0});
    }

    auto* segment_item = scene_->addLine(visible_line, color_pen);
    segment_item->setZValue(repeated_pass ? 1.52 : 1.4);
}

QColor SimulationView::TrajectorySegmentColor(
    const client_gateway::SimulatedDroneTrailSegment& segment) const {
    QColor base = TrajectoryColor(segment.drone_id);
    if (segment.edge_pass_index <= 1) {
        base.setAlpha(215);
        return base;
    }

    const int repeat_index = segment.edge_pass_index - 1;
    const int saturation = std::min(255, base.saturation() + std::min(120, 80 + repeat_index * 18));
    const int value = std::max(42, base.value() - std::min(205, 120 + repeat_index * 28));
    QColor repeated = QColor::fromHsv(base.hue(), saturation, value);
    repeated.setAlpha(255);
    return repeated;
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

    start_action_ = tool_strip_->addAction(tr("Start"));
    pause_action_ = tool_strip_->addAction(tr("Pause"));
    resume_action_ = tool_strip_->addAction(tr("Resume"));
    stop_action_ = tool_strip_->addAction(tr("Stop"));
    tool_strip_->addSeparator();
    fit_action_ = tool_strip_->addAction(tr("Fit"));
    layout->addWidget(tool_strip_);

    view_ = new SimulationView(this);
    layout->addWidget(view_, 1);

    connect(start_action_, &QAction::triggered, this, &SimulationPage::StartSimulation);
    connect(pause_action_, &QAction::triggered, this, &SimulationPage::PauseSimulation);
    connect(resume_action_, &QAction::triggered, this, &SimulationPage::ResumeSimulation);
    connect(stop_action_, &QAction::triggered, this, &SimulationPage::StopSimulation);
    connect(fit_action_, &QAction::triggered, view_, &SimulationView::FitToWorkspace);

    connect(&MissionWorkspaceRuntime(), &MissionWorkspaceService::SigWorkspaceChanged, this,
            &SimulationPage::OnWorkspaceChanged);
    connect(&client_gateway::ClientGatewayRuntime().SwarmRuntime(),
            &client_gateway::ISwarmRuntimeClient::SigMissionSimulationFrame, this,
            &SimulationPage::OnSimulationFrame);

    OnWorkspaceChanged(MissionWorkspaceRuntime().ActiveWorkspace());
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

void SimulationPage::PauseSimulation() {
    client_gateway::ClientGatewayRuntime().SwarmRuntime().PauseMissionSimulation();
}

void SimulationPage::ResumeSimulation() {
    client_gateway::ClientGatewayRuntime().SwarmRuntime().ResumeMissionSimulation();
}

void SimulationPage::StopSimulation() {
    client_gateway::ClientGatewayRuntime().SwarmRuntime().StopMissionSimulation();
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
    const bool running = state == client_gateway::MissionSimulationState::kRunning;
    const bool paused = state == client_gateway::MissionSimulationState::kPaused;
    start_action_->setEnabled(!running);
    pause_action_->setEnabled(running);
    resume_action_->setEnabled(paused);
    stop_action_->setEnabled(running || paused);
}

}  // namespace app::mission
