#pragma once

#include <QString>
#include <QWidget>
#include <string_view>

class QLabel;
class QHBoxLayout;
class QVBoxLayout;

namespace app::ui {

class PanelWidget : public QWidget {
   public:
    PanelWidget(const QString& title, std::string_view object_name, QWidget* parent = nullptr);
    ~PanelWidget() override = default;

    void SetTitle(const QString& title);

   protected:
    void AddHeaderWidget(QWidget* widget);

    [[nodiscard]] QWidget* ContentWidget() const;
    [[nodiscard]] QVBoxLayout* ContentLayout() const;

   private:
    QLabel* title_label_{nullptr};
    QHBoxLayout* header_layout_{nullptr};
    QWidget* content_{nullptr};
    QVBoxLayout* content_layout_{nullptr};
};

}  // namespace app::ui
