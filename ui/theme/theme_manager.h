#pragma once

#include <QObject>
#include <QPalette>
#include <QStringList>

#include "theme/theme_colors.h"
#include "theme/theme_loader.h"

namespace app::ui::theme {

class ThemeManager final : public QObject {
    Q_OBJECT
   public:
    static ThemeManager& Instance();

    [[nodiscard]] ThemeId CurrentThemeId() const;
    [[nodiscard]] ThemeColors CurrentColors() const;

    void Apply(ThemeId theme_id);
    void Toggle();

   signals:
    void SigThemeChanged(ThemeId theme_id);

   private:
    ThemeManager() = default;

    void ApplyToApp();
    [[nodiscard]] static QStringList QssPaths();
    static QPalette MakePalette(const ThemeColors& colors);

    bool wired_{false};
    ThemeId theme_id_{ThemeId::kDark};
};

}  // namespace app::ui::theme
