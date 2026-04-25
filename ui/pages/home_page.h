#pragma once

#include <QWidget>

namespace app::ui {

class HomePage final : public QWidget {
    Q_OBJECT
   public:
    explicit HomePage(QWidget* parent = nullptr);
    ~HomePage() override = default;
};

}  // namespace app::ui
