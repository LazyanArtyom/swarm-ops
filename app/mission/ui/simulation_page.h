#pragma once

#include <QGraphicsView>
#include <QHash>
#include <QWidget>

#include "app/client_gateway/client_gateway_types.h"
#include "app/mission/mission_workspace.h"

class QAction;
class QGraphicsLineItem;
class QGraphicsPixmapItem;
class QGraphicsScene;
class QLabel;
class QSlider;
class QToolBar;

namespace app::mission {

class SimulationView final : public QGraphicsView {
    Q_OBJECT

   public:
    explicit SimulationView(QWidget* parent = nullptr);

    void SetWorkspace(const MissionWorkspace& workspace);
    void ApplyFrame(const client_gateway::MissionSimulationFrame& frame);
    void FitToWorkspace();

   protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

   private:
    void RebuildScene();
    void ScheduleFitToWorkspace();
    void RenderWorkspaceDrones();
    void EnsureDroneItem(const client_gateway::SimulatedDronePosition& drone);
    void DrawTrajectorySegment(const client_gateway::SimulatedDroneTrailSegment& segment);
    void UpdatePrimaryTrajectory(const client_gateway::SimulatedDroneTrailSegment& segment,
                                 const QString& edge_key, const QLineF& edge_line);
    void UpdateRepeatedTrajectory(const client_gateway::SimulatedDroneTrailSegment& segment,
                                  const QString& edge_key, const QLineF& edge_line);
    [[nodiscard]] bool TryEdgeLine(const client_gateway::SimulatedDroneTrailSegment& segment,
                                   QString* edge_key, QLineF* edge_line) const;
    [[nodiscard]] QColor TrajectorySegmentColor(
        const client_gateway::SimulatedDroneTrailSegment& segment) const;
    [[nodiscard]] int SecondaryTrajectoryLane(const QString& edge_key, const QString& drone_id);

    QGraphicsScene* scene_{nullptr};
    MissionWorkspace workspace_;
    QGraphicsPixmapItem* background_item_{nullptr};
    QHash<QString, QPointF> node_positions_;
    QHash<QString, QGraphicsPixmapItem*> drone_items_;
    QHash<QString, QGraphicsLineItem*> primary_trajectory_items_;
    QHash<QString, QGraphicsLineItem*> secondary_trajectory_items_;
    QHash<QString, int> secondary_trajectory_lanes_;
    QHash<QString, int> next_secondary_lane_by_edge_;
    bool fit_pending_{true};
};

class SimulationPage final : public QWidget {
    Q_OBJECT

   public:
    explicit SimulationPage(QWidget* parent = nullptr);

   private:
    void ToggleRunState();
    void StopSimulation();
    void StartSimulation();
    void SetSimulationSpeed(int slider_value);
    void OnWorkspaceChanged(const MissionWorkspace& workspace);
    void OnSimulationFrame(const client_gateway::MissionSimulationFrame& frame);
    void RefreshToolbarState(client_gateway::MissionSimulationState state);
    void RefreshSpeedLabel();

    QToolBar* tool_strip_{nullptr};
    SimulationView* view_{nullptr};
    QAction* run_action_{nullptr};
    QAction* stop_action_{nullptr};
    QAction* fit_action_{nullptr};
    QLabel* speed_label_{nullptr};
    QSlider* speed_slider_{nullptr};
    client_gateway::MissionSimulationState simulation_state_{
        client_gateway::MissionSimulationState::kIdle};
};

}  // namespace app::mission
