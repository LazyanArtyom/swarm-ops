#include "app/controllers/navigation_controller.h"

#include <QList>

#include "app/app_services.h"
#include "ui/panels/central_panel.h"
#include "ui/panels/navigation_panel.h"
#include "ui/workspace/page_registry.h"

namespace app::controllers {

NavigationController::NavigationController(const AppServices& services,
                                           ui::NavigationPanel* navigation_panel,
                                           ui::CentralPanel* central_panel, QObject* parent)
    : QObject(parent),
      services_(services),
      navigation_panel_(navigation_panel),
      central_panel_(central_panel) {
    WireNavigation();
}

void NavigationController::RefreshNavigationItems() {
    if (navigation_panel_ == nullptr) {
        return;
    }

    QList<ui::NavigationPanel::Item> items;
    for (const ui::workspace::PageDescriptor& page : services_.Pages().NavigationPages()) {
        items.push_back({
            .page_key = page.page_key,
            .label = page.title,
            .icon = page.icon,
            .category = page.category,
            .shortcut = page.shortcut,
        });
    }
    navigation_panel_->SetItems(items);
}

void NavigationController::OpenHome() {
    const QString page_key = services_.Pages().DefaultPageKey();
    if (page_key.isEmpty()) {
        return;
    }

    if (central_panel_ != nullptr) {
        central_panel_->ShowPage(page_key);
    }

    if (navigation_panel_ != nullptr) {
        navigation_panel_->SelectPage(page_key);
    }
}

void NavigationController::WireNavigation() {
    if (central_panel_ == nullptr || navigation_panel_ == nullptr) {
        return;
    }

    connect(navigation_panel_, &ui::NavigationPanel::SigPageRequested, this,
            [this](const QString& page_key) {
                if (central_panel_ != nullptr) {
                    central_panel_->ShowPage(page_key);
                }
            });

    connect(central_panel_, &ui::CentralPanel::SigCurrentPageChanged, this,
            [this](const QString& page_key) {
                if (navigation_panel_ != nullptr) {
                    navigation_panel_->SelectPage(page_key);
                }
            });
}

}  // namespace app::controllers
