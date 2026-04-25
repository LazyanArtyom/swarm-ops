#pragma once

#include <QDialog>

namespace app::ui {

class AboutDialog final : public QDialog {
    Q_OBJECT
   public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override = default;
};

}  // namespace app::ui
