#include "widgets/settings_form_row.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>

#include "object_names.h"
#include "theme/theme_metrics.h"

namespace app::ui {
namespace {

constexpr int kSettingsComboMaxVisibleRows = 8;

void ConfigureComboBox(QComboBox* combo, const theme::ThemeMetricsData& metrics) {
    if (combo == nullptr) {
        return;
    }

    combo->setObjectName(QString::fromLatin1(object_names::kSettingsField));
    combo->setProperty("settingsRole", QStringLiteral("field"));
    combo->setMaxVisibleItems(kSettingsComboMaxVisibleRows);
    combo->setMinimumWidth(metrics.settings_field_min_width_px);
    combo->setFixedHeight(metrics.settings_field_height_px);
    if (combo->view() != nullptr) {
        combo->view()->setObjectName(QString::fromLatin1(object_names::kSettingsPopup));
        combo->view()->setMinimumHeight(metrics.settings_popup_min_height_px);
    }
    combo->setStyleSheet(QString());
}

void ConfigureField(QWidget* field, const theme::ThemeMetricsData& metrics) {
    if (field == nullptr) {
        return;
    }

    field->setProperty("settingsRole", QStringLiteral("field"));
    field->setMinimumWidth(metrics.settings_field_min_width_px);
    field->setFixedHeight(metrics.settings_field_height_px);

    if (auto* combo = qobject_cast<QComboBox*>(field); combo != nullptr) {
        ConfigureComboBox(combo, metrics);
    }
}

}  // namespace

SettingsFormRow::SettingsFormRow(const QString& label_text, QWidget* field, QWidget* parent)
    : QWidget(parent) {
    setObjectName(QString::fromLatin1(object_names::kSettingsRow));
    setProperty("uiComponent", QStringLiteral("settings-row"));

    auto* row_layout = new QHBoxLayout(this);
    row_layout->setContentsMargins(0, 0, 0, 0);

    label_ = new QLabel(label_text, this);
    label_->setObjectName(QString::fromLatin1(object_names::kSettingsLabel));
    label_->setProperty("uiComponent", QStringLiteral("settings-label"));
    label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    field_host_ = new QWidget(this);
    field_host_->setObjectName(QString::fromLatin1(object_names::kSettingsFieldHost));
    field_host_->setProperty("uiComponent", QStringLiteral("settings-field-host"));

    field_host_layout_ = new QHBoxLayout(field_host_);
    field_host_layout_->setContentsMargins(0, 0, 0, 0);
    field_host_layout_->setSpacing(0);

    row_layout->addWidget(label_);
    row_layout->addWidget(field_host_, 1);

    SetField(field);
    ApplyMetrics(theme::ThemeMetrics::Instance().Current());
}

void SettingsFormRow::ApplyMetrics(const theme::ThemeMetricsData& metrics) {
    auto* row_layout = qobject_cast<QHBoxLayout*>(layout());
    if (row_layout != nullptr) {
        row_layout->setSpacing(metrics.spacing_lg_px);
    }

    if (label_ != nullptr) {
        label_->setFixedWidth(metrics.settings_label_width_px);
        label_->setFixedHeight(metrics.settings_field_height_px);
    }

    setFixedHeight(metrics.settings_field_height_px);
    ConfigureField(field_, metrics);
}

QLabel* SettingsFormRow::Label() const {
    return label_;
}

QWidget* SettingsFormRow::Field() const {
    return field_;
}

void SettingsFormRow::SetField(QWidget* field) {
    if (field_host_layout_ == nullptr || field == nullptr) {
        return;
    }

    field_ = field;
    field_->setParent(field_host_);
    field_host_layout_->addWidget(field_);
}

}  // namespace app::ui
