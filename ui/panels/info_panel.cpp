#include "panels/info_panel.h"

#include <QLabel>
#include <QScrollArea>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

#include "object_names.h"
#include "theme/theme_metrics.h"

namespace app::ui {

InfoPanel::InfoPanel(QWidget* parent) : PanelWidget(tr("Info"), object_names::kInfoPanel, parent) {
    SetupUi();
}

void InfoPanel::SetContent(QWidget* widget) {
    if (widget == nullptr) {
        ClearContent();
        return;
    }

    if (current_content_ == widget) {
        ShowEmptyState(false);
        return;
    }

    ClearContent();
    current_content_ = widget;
    current_content_->setParent(content_host_);
    content_layout_->addWidget(current_content_);
    ShowEmptyState(false);
}

void InfoPanel::ClearContent() {
    if (current_content_ != nullptr) {
        content_layout_->removeWidget(current_content_);
        current_content_->deleteLater();
        current_content_.clear();
    }

    ShowEmptyState(true);
}

void InfoPanel::SetEmptyState(const QString& title, const QString& message) {
    if (empty_title_ != nullptr) {
        empty_title_->setText(title);
    }
    if (empty_message_ != nullptr) {
        empty_message_->setText(message);
    }
}

QWidget* InfoPanel::CurrentContent() const {
    return current_content_;
}

void InfoPanel::SetupUi() {
    const auto metrics = theme::ThemeMetrics::Instance().Current();

    auto* v_outer_layout = ContentLayout();
    v_outer_layout->setContentsMargins(metrics.spacing_sm_px, metrics.spacing_sm_px,
                                       metrics.spacing_sm_px, metrics.spacing_sm_px);
    v_outer_layout->setSpacing(metrics.spacing_sm_px);

    scroll_area_ = new QScrollArea(ContentWidget());
    scroll_area_->setAttribute(Qt::WA_StyledBackground, true);
    scroll_area_->setProperty("uiComponent", QStringLiteral("panel-scroll-area"));
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setFrameShape(QFrame::NoFrame);
    v_outer_layout->addWidget(scroll_area_);

    content_host_ = new QWidget(scroll_area_);
    content_host_->setAttribute(Qt::WA_StyledBackground, true);
    content_host_->setProperty("uiComponent", QStringLiteral("panel-content"));
    content_layout_ = new QVBoxLayout(content_host_);
    content_layout_->setContentsMargins(0, 0, 0, 0);
    content_layout_->setSpacing(metrics.spacing_sm_px);

    empty_state_ = new QWidget(content_host_);
    empty_state_->setObjectName(QString::fromLatin1(object_names::kInfoEmptyState));
    empty_state_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* empty_layout = new QVBoxLayout(empty_state_);
    empty_layout->setContentsMargins(metrics.spacing_md_px, metrics.spacing_lg_px,
                                     metrics.spacing_md_px, metrics.spacing_lg_px);
    empty_layout->setSpacing(metrics.spacing_sm_px);
    empty_layout->addStretch(1);

    empty_title_ = new QLabel(empty_state_);
    empty_title_->setObjectName(QString::fromLatin1(object_names::kInfoEmptyTitle));
    empty_title_->setAlignment(Qt::AlignCenter);

    empty_message_ = new QLabel(empty_state_);
    empty_message_->setObjectName(QString::fromLatin1(object_names::kInfoEmptyMessage));
    empty_message_->setProperty("role", QStringLiteral("muted"));
    empty_message_->setAlignment(Qt::AlignCenter);
    empty_message_->setWordWrap(true);

    empty_layout->addWidget(empty_title_);
    empty_layout->addWidget(empty_message_);
    empty_layout->addStretch(1);

    content_layout_->addWidget(empty_state_, 1);
    scroll_area_->setWidget(content_host_);

    SetEmptyState(tr("No Selection"), tr("Select an item to view details."));
}

void InfoPanel::ShowEmptyState(bool visible) {
    if (empty_state_ != nullptr) {
        empty_state_->setVisible(visible);
    }
}

}  // namespace app::ui
