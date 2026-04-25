#pragma once

#include "app/modules/app_module.h"

namespace app::modules {

class HomeModule final : public IAppModule {
   public:
    void RegisterPages(ui::workspace::PageRegistry& registry) override;
};

}  // namespace app::modules
