#include "pages/home_page.h"

#include <QLabel>
#include <QVBoxLayout>

#include "theme/theme_metrics.h"

namespace app::ui {

HomePage::HomePage(QWidget* parent) : QWidget(parent) {
    const auto metrics = theme::ThemeMetrics::Instance().Current();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(metrics.spacing_md_px, metrics.spacing_md_px, metrics.spacing_md_px,
                               metrics.spacing_md_px);
    layout->addWidget(new QLabel(QStringLiteral("Home Page"), this));
}

}  // namespace app::ui
