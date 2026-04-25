#include "app/modules/home_module.h"

#include <QCoreApplication>

#include "ui/ids.h"
#include "ui/pages/home_page.h"
#include "ui/workspace/page_registry.h"

namespace app::modules {

void HomeModule::RegisterPages(ui::workspace::PageRegistry& registry) {
    registry.RegisterPage({
        .page_key = ui::ids::kPageHome,
        .title = QCoreApplication::translate("HomeModule", "Home"),
        .category = QCoreApplication::translate("HomeModule", "General"),
        .factory = [](QWidget* parent) { return new ui::HomePage(parent); },
        .navigation_visible = true,
    });
}

}  // namespace app::modules
