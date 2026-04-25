#include "panels/navigation_panel.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QSignalBlocker>
#include <QStringList>
#include <QVBoxLayout>

#include "object_names.h"
#include "theme/theme_metrics.h"

namespace app::ui {
namespace {

QString ItemToolTip(const NavigationPanel::Item& item) {
    QStringList parts;
    if (!item.category.isEmpty()) {
        parts.push_back(item.category);
    }
    if (!item.shortcut.isEmpty()) {
        parts.push_back(item.shortcut.toString(QKeySequence::NativeText));
    }
    return parts.join(QStringLiteral(" - "));
}

}  // namespace

NavigationPanel::NavigationPanel(QWidget* parent)
    : PanelWidget(tr("Navigation"), object_names::kNavigationPanel, parent) {
    const auto metrics = theme::ThemeMetrics::Instance().Current();

    auto* layout = ContentLayout();
    layout->setContentsMargins(metrics.spacing_sm_px, metrics.spacing_sm_px, metrics.spacing_sm_px,
                               metrics.spacing_sm_px);

    list_ = new QListWidget(ContentWidget());
    list_->setObjectName(QString::fromLatin1(object_names::kNavigationList));
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->setUniformItemSizes(true);
    list_->setAlternatingRowColors(false);
    list_->setSpacing(metrics.spacing_xs_px);
    layout->addWidget(list_);

    connect(list_, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* current, QListWidgetItem*) { OnCurrentItemChanged(current); });
}

void NavigationPanel::SetItems(const QList<Item>& items) {
    if (list_ == nullptr) {
        return;
    }

    const QSignalBlocker blocker(list_);
    list_->clear();

    for (const Item& item : items) {
        if (item.page_key.isEmpty() || item.label.isEmpty()) {
            continue;
        }

        auto* widget_item = new QListWidgetItem(item.icon, item.label, list_);
        widget_item->setData(Qt::UserRole, item.page_key);
        const QString tool_tip = ItemToolTip(item);
        if (!tool_tip.isEmpty()) {
            widget_item->setToolTip(tool_tip);
        }
    }

    if (list_->count() > 0) {
        list_->setCurrentRow(0);
    }
}

void NavigationPanel::SelectPage(const QString& page_key) {
    if (list_ == nullptr || page_key.isEmpty()) {
        return;
    }

    for (int i = 0; i < list_->count(); ++i) {
        QListWidgetItem* item = list_->item(i);
        if (item == nullptr) {
            continue;
        }
        if (item->data(Qt::UserRole).toString() != page_key) {
            continue;
        }

        const QSignalBlocker blocker(list_);
        list_->setCurrentRow(i);
        return;
    }
}

void NavigationPanel::OnCurrentItemChanged(QListWidgetItem* current) {
    if (current == nullptr) {
        return;
    }

    const QString page_key = current->data(Qt::UserRole).toString();
    if (!page_key.isEmpty()) {
        emit SigPageRequested(page_key);
    }
}

}  // namespace app::ui
