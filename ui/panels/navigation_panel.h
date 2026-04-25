#pragma once

#include <QIcon>
#include <QKeySequence>

#include "panels/panel_widget.h"

class QListWidget;
class QListWidgetItem;

namespace app::ui {

class NavigationPanel final : public PanelWidget {
    Q_OBJECT

   public:
    struct Item final {
        QString page_key;
        QString label;
        QIcon icon;
        QString category;
        QKeySequence shortcut;
    };

    explicit NavigationPanel(QWidget* parent = nullptr);
    ~NavigationPanel() override = default;

    void SetItems(const QList<Item>& items);
    void SelectPage(const QString& page_key);

   signals:
    void SigPageRequested(const QString& page_key);

   private:
    void OnCurrentItemChanged(QListWidgetItem* current);

    QListWidget* list_{nullptr};
};

}  // namespace app::ui
