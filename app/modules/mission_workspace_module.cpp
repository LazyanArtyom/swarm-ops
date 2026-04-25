#include "app/modules/mission_workspace_module.h"

#include <QCoreApplication>

#include "app/mission/ui/graph_editor_page.h"
#include "app/mission/ui/map_selection_page.h"
#include "ui/ids.h"
#include "ui/workspace/page_registry.h"

namespace app::modules {

void MissionWorkspaceModule::RegisterPages(ui::workspace::PageRegistry& registry) {
    registry.RegisterPage({
        .page_key = ui::ids::kPageMissionMapSelection,
        .title = QCoreApplication::translate("MissionWorkspaceModule", "Map Selection"),
        .category = QCoreApplication::translate("MissionWorkspaceModule", "Mission"),
        .factory = [](QWidget* parent) { return new mission::MapSelectionPage(parent); },
        .navigation_visible = true,
    });

    registry.RegisterPage({
        .page_key = ui::ids::kPageMissionGraphEditor,
        .title = QCoreApplication::translate("MissionWorkspaceModule", "Graph Editor"),
        .category = QCoreApplication::translate("MissionWorkspaceModule", "Mission"),
        .factory = [](QWidget* parent) { return new mission::GraphEditorPage(parent); },
        .navigation_visible = true,
    });
}

}  // namespace app::modules
