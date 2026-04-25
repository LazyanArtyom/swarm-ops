#pragma once

#include <QMenuBar>

namespace app::ui::actions {
class AppActions;
}

namespace app::ui::chrome {

class MainMenuBar final : public QMenuBar {
    Q_OBJECT

   public:
    explicit MainMenuBar(actions::AppActions* app_actions, QWidget* parent = nullptr);
    ~MainMenuBar() override = default;
};

}  // namespace app::ui::chrome
