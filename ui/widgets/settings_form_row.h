#pragma once

#include <QWidget>

class QLabel;
class QHBoxLayout;

namespace app::ui::theme {
struct ThemeMetricsData;
}

namespace app::ui {

class SettingsFormRow final : public QWidget {
    Q_OBJECT

   public:
    SettingsFormRow(const QString& label_text, QWidget* field, QWidget* parent = nullptr);
    ~SettingsFormRow() override = default;

    void ApplyMetrics(const theme::ThemeMetricsData& metrics);

    [[nodiscard]] QLabel* Label() const;
    [[nodiscard]] QWidget* Field() const;

   private:
    void SetField(QWidget* field);

    QLabel* label_{nullptr};
    QWidget* field_host_{nullptr};
    QHBoxLayout* field_host_layout_{nullptr};
    QWidget* field_{nullptr};
};

}  // namespace app::ui
