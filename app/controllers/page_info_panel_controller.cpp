#include "app/controllers/page_info_panel_controller.h"

#include <QWidget>

#include "ui/panels/central_panel.h"
#include "ui/panels/info_panel.h"
#include "ui/panels/info_panel_page.h"

namespace app::controllers {

PageInfoPanelController::PageInfoPanelController(ui::CentralPanel* central_panel,
                                                 ui::InfoPanel* info_panel, QObject* parent)
    : QObject(parent), central_panel_(central_panel), info_panel_(info_panel) {}

void PageInfoPanelController::Wire() {
    if (central_panel_ == nullptr) {
        return;
    }

    connect(central_panel_, &ui::CentralPanel::SigCurrentPageChanged, this,
            [this](const QString&) { Refresh(); });
    Refresh();
}

void PageInfoPanelController::Refresh() {
    if (central_panel_ == nullptr || info_panel_ == nullptr) {
        return;
    }

    QWidget* page = central_panel_->CurrentPage();
    auto* info_page = qobject_cast<ui::IInfoPanelPage*>(page);
    if (info_page == nullptr) {
        info_panel_->ClearContent();
        return;
    }

    QWidget* content = info_page->CreateInfoPanelWidget(info_panel_);
    if (content == nullptr) {
        info_panel_->ClearContent();
        return;
    }

    info_panel_->SetContent(content);
}

}  // namespace app::controllers
