#include "app/controllers/workspace_controller.h"

#include "ui/panels/central_panel.h"

namespace app::controllers {
WorkspaceController::WorkspaceController(ui::workspace::PageRegistry& page_registry,
                                         QObject* parent)
    : QObject(parent), page_registry_(page_registry) {}

ui::CentralPanel* WorkspaceController::CreateCentralPanel(QWidget* parent) {
    if (central_panel_ != nullptr) {
        return central_panel_;
    }

    central_panel_ = new ui::CentralPanel(parent);
    central_panel_->SetPageRegistry(&page_registry_);
    return central_panel_;
}

ui::CentralPanel* WorkspaceController::CentralPanelWidget() const {
    return central_panel_;
}

}  // namespace app::controllers
