#pragma once

#include <QPointer>
#include <QString>

#include "panels/panel_widget.h"

class QLabel;
class QScrollArea;
class QVBoxLayout;

namespace app::ui {

class InfoPanel final : public PanelWidget {
    Q_OBJECT

   public:
    explicit InfoPanel(QWidget* parent = nullptr);
    ~InfoPanel() override = default;

    void SetContent(QWidget* widget);
    void ClearContent();
    void SetEmptyState(const QString& title, const QString& message);

    [[nodiscard]] QWidget* CurrentContent() const;

   private:
    void SetupUi();
    void ShowEmptyState(bool visible);

    QScrollArea* scroll_area_{nullptr};
    QWidget* content_host_{nullptr};
    QVBoxLayout* content_layout_{nullptr};
    QWidget* empty_state_{nullptr};
    QLabel* empty_title_{nullptr};
    QLabel* empty_message_{nullptr};
    QPointer<QWidget> current_content_;
};

}  // namespace app::ui
