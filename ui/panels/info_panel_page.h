#pragma once

#include <QtPlugin>

class QWidget;

namespace app::ui {

class IInfoPanelPage {
   public:
    virtual ~IInfoPanelPage() = default;

    [[nodiscard]] virtual QWidget* CreateInfoPanelWidget(QWidget* parent) = 0;
};

}  // namespace app::ui

#define APP_UI_INFO_PANEL_PAGE_IID "app.ui.IInfoPanelPage"

Q_DECLARE_INTERFACE(app::ui::IInfoPanelPage, APP_UI_INFO_PANEL_PAGE_IID)
