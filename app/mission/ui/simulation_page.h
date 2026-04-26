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
    [[nodiscard]] QColor TrajectorySegmentColor(
        const client_gateway::SimulatedDroneTrailSegment& segment) const;

    QGraphicsScene* scene_{nullptr};
    MissionWorkspace workspace_;
    QGraphicsPixmapItem* background_item_{nullptr};
    QHash<QString, QGraphicsPixmapItem*> drone_items_;
    bool fit_pending_{true};
};

class SimulationPage final : public QWidget {
    Q_OBJECT

   public:
    explicit SimulationPage(QWidget* parent = nullptr);

   private:
    void StartSimulation();
    void PauseSimulation();
    void ResumeSimulation();
    void StopSimulation();
    void OnWorkspaceChanged(const MissionWorkspace& workspace);
    void OnSimulationFrame(const client_gateway::MissionSimulationFrame& frame);
    void RefreshToolbarState(client_gateway::MissionSimulationState state);

    QToolBar* tool_strip_{nullptr};
    SimulationView* view_{nullptr};
    QAction* start_action_{nullptr};
    QAction* pause_action_{nullptr};
    QAction* resume_action_{nullptr};
    QAction* stop_action_{nullptr};
    QAction* fit_action_{nullptr};
};

}  // namespace app::mission
