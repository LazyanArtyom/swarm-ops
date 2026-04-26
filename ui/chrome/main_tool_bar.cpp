#include "chrome/main_tool_bar.h"

#include <QActionEvent>
#include <QSize>
#include <QToolButton>

#include "actions/app_actions.h"
#include "object_names.h"
#include "theme/theme_metrics.h"

namespace app::ui::chrome {
namespace {

constexpr int kToolTipDurationMs = 2500;

}  // namespace

MainToolBar::MainToolBar(actions::AppActions* app_actions, QWidget* parent)
    : QToolBar(tr("Main"), parent) {
    setObjectName(QString::fromLatin1(object_names::kMainToolBar));
    setProperty("uiComponent", QStringLiteral("toolbar-chrome"));
    setMovable(false);
    setFloatable(false);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    addAction(app_actions->NewAction());
    addAction(app_actions->OpenAction());
    addAction(app_actions->SaveAction());

    RefreshMetrics();
}

void MainToolBar::RefreshMetrics() {
    const auto metrics = theme::ThemeMetrics::Instance().Current();
    setIconSize(QSize(metrics.icon_md_px, metrics.icon_md_px));
    ApplyToolButtonCursors();
}

void MainToolBar::actionEvent(QActionEvent* event) {
    QToolBar::actionEvent(event);
    ApplyToolButtonCursors();
}

void MainToolBar::ApplyToolButtonCursors() {
    const auto buttons = findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto* button : buttons) {
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTipDuration(kToolTipDurationMs);
    }
}

}  // namespace app::ui::chrome
