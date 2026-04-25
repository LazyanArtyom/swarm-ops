#pragma once

#include <QObject>

class QWidget;

namespace app::ui {
class CentralPanel;
namespace workspace {
class PageRegistry;
}  // namespace workspace
}  // namespace app::ui

namespace app::controllers {

class WorkspaceController final : public QObject {
    Q_OBJECT

   public:
    explicit WorkspaceController(ui::workspace::PageRegistry& page_registry,
                                 QObject* parent = nullptr);
    ~WorkspaceController() override = default;

    ui::CentralPanel* CreateCentralPanel(QWidget* parent = nullptr);
    [[nodiscard]] ui::CentralPanel* CentralPanelWidget() const;

   private:
    ui::workspace::PageRegistry& page_registry_;
    ui::CentralPanel* central_panel_{nullptr};
};

}  // namespace app::controllers
