#include "panels/panel_widget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "object_names.h"

namespace app::ui {

PanelWidget::PanelWidget(const QString& title, std::string_view object_name, QWidget* parent)
    : QWidget(parent) {
    if (!object_name.empty()) {
        setObjectName(
            QString::fromLatin1(object_name.data(), static_cast<qsizetype>(object_name.size())));
    }
    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("uiComponent", QStringLiteral("panel-shell"));

    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->setSpacing(0);

    auto* header = new QWidget(this);
    header->setObjectName(QString::fromLatin1(object_names::kPanelHeader));
    header->setAttribute(Qt::WA_StyledBackground, true);
    header->setProperty("uiComponent", QStringLiteral("panel-header"));

    header_layout_ = new QHBoxLayout(header);
    header_layout_->setContentsMargins(0, 0, 0, 0);
    header_layout_->setSpacing(0);

    title_label_ = new QLabel(title, header);
    title_label_->setObjectName(QString::fromLatin1(object_names::kPanelTitle));
    title_label_->setProperty("uiComponent", QStringLiteral("panel-title"));
    header_layout_->addWidget(title_label_);
    header_layout_->addStretch(1);

    content_ = new QWidget(this);
    content_->setObjectName(QString::fromLatin1(object_names::kPanelContent));
    content_->setAttribute(Qt::WA_StyledBackground, true);
    content_->setProperty("uiComponent", QStringLiteral("panel-content"));

    content_layout_ = new QVBoxLayout(content_);
    content_layout_->setContentsMargins(0, 0, 0, 0);
    content_layout_->setSpacing(0);

    root_layout->addWidget(header);
    root_layout->addWidget(content_, 1);
}

void PanelWidget::SetTitle(const QString& title) {
    if (title_label_ != nullptr) {
        title_label_->setText(title);
    }
}

void PanelWidget::AddHeaderWidget(QWidget* widget) {
    if (header_layout_ != nullptr && widget != nullptr) {
        header_layout_->addWidget(widget);
    }
}

QWidget* PanelWidget::ContentWidget() const {
    return content_;
}

QVBoxLayout* PanelWidget::ContentLayout() const {
    return content_layout_;
}

}  // namespace app::ui
