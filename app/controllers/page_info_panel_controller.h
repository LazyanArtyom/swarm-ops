#pragma once

#include <QObject>

namespace app::ui {
class CentralPanel;
class InfoPanel;
}  // namespace app::ui

namespace app::controllers {

class PageInfoPanelController final : public QObject {
    Q_OBJECT

   public:
    PageInfoPanelController(ui::CentralPanel* central_panel, ui::InfoPanel* info_panel,
                            QObject* parent = nullptr);

    void Wire();
    void Refresh();

   private:
    ui::CentralPanel* central_panel_{nullptr};
    ui::InfoPanel* info_panel_{nullptr};
};

}  // namespace app::controllers
