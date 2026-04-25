#pragma once

#include <QToolBar>

class QActionEvent;

namespace app::ui::actions {
class AppActions;
}

namespace app::ui::chrome {

class MainToolBar final : public QToolBar {
    Q_OBJECT

   public:
    explicit MainToolBar(actions::AppActions* app_actions, QWidget* parent = nullptr);
    ~MainToolBar() override = default;

    void RefreshMetrics();

   protected:
    void actionEvent(QActionEvent* event) override;

   private:
    void ApplyToolButtonCursors();
};

}  // namespace app::ui::chrome
