#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class QAction;
class QToolButton;

#include "theme/theme_colors.h"

namespace app::ui::theme {

/**
 * @brief Central registry for themed icons (base + _dark/_light suffix).
 *
 * Register once:
 *   ThemeIcons::Instance().BindAction(action, "open");
 *
 * ThemeManager calls Refresh(theme_id) on theme changes.
 * BindAction/BindToolButton also apply immediately (so icons appear on startup).
 */
class ThemeIcons final : public QObject {
    Q_OBJECT

   public:
    static ThemeIcons& Instance();

    void BindAction(QAction* action, const QString& icon_base_name);
    void BindToolButton(QToolButton* button, const QString& icon_base_name);

    void Refresh(ThemeId theme_id);

   private:
    ThemeIcons() = default;

    struct Entry final {
        QPointer<QObject> obj;
        QString base_name;
    };

    QList<Entry> entries_;

    static QString ThemedPath(const QString& base_name, ThemeId theme_id);
    void CleanupDeadEntries();

    // Apply icon to a single object (QAction/QToolButton). Returns false if unsupported/null.
    static bool ApplyIcon(QObject* obj, const QString& base_name, ThemeId theme_id);
};

}  // namespace app::ui::theme
