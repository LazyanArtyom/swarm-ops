#include "theme/theme_icons.h"

#include <QAction>
#include <QToolButton>

#include "theme/theme_manager.h"

namespace app::ui::theme {
namespace {

QString Shade(ThemeId theme_id) {
    return (theme_id == ThemeId::kDark) ? QStringLiteral("dark") : QStringLiteral("light");
}

}  // namespace

ThemeIcons& ThemeIcons::Instance() {
    static ThemeIcons instance;
    return instance;
}

QString ThemeIcons::ThemedPath(const QString& base_name, ThemeId theme_id) {
    return QStringLiteral(":/theme/icons/%1_%2.svg").arg(base_name, Shade(theme_id));
}

void ThemeIcons::CleanupDeadEntries() {
    for (auto i = entries_.size() - 1; i >= 0; --i) {
        if (entries_[i].obj.isNull()) {
            entries_.removeAt(i);
        }
    }
}

bool ThemeIcons::ApplyIcon(QObject* obj, const QString& base_name, ThemeId theme_id) {
    if (obj == nullptr || base_name.isEmpty()) {
        return false;
    }

    const QIcon icon(ThemedPath(base_name, theme_id));

    if (auto* action = qobject_cast<QAction*>(obj)) {
        action->setIcon(icon);
        return true;
    }
    if (auto* button = qobject_cast<QToolButton*>(obj)) {
        button->setIcon(icon);
        return true;
    }

    return false;
}

void ThemeIcons::BindAction(QAction* action, const QString& icon_base_name) {
    if (action == nullptr || icon_base_name.isEmpty()) {
        return;
    }

    entries_.push_back(Entry{.obj = action, .base_name = icon_base_name});

    const ThemeId theme_id = ThemeManager::Instance().CurrentThemeId();
    ApplyIcon(action, icon_base_name, theme_id);
}

void ThemeIcons::BindToolButton(QToolButton* button, const QString& icon_base_name) {
    if (button == nullptr || icon_base_name.isEmpty()) {
        return;
    }

    entries_.push_back(Entry{.obj = button, .base_name = icon_base_name});

    const ThemeId theme_id = ThemeManager::Instance().CurrentThemeId();
    ApplyIcon(button, icon_base_name, theme_id);
}

void ThemeIcons::Refresh(ThemeId theme_id) {
    CleanupDeadEntries();

    for (const auto& entry : entries_) {
        QObject* obj = entry.obj.data();
        if (obj == nullptr) {
            continue;
        }
        ApplyIcon(obj, entry.base_name, theme_id);
    }
}

}  // namespace app::ui::theme
