#pragma once

#include <QByteArray>

namespace app::ui::workspace {

class IPageLifecycle {
   public:
    virtual ~IPageLifecycle() = default;

    virtual void OnActivate();
    virtual void OnDeactivate();
    [[nodiscard]] virtual bool CanClose() const;
    [[nodiscard]] virtual QByteArray SaveState() const;
    virtual void RestoreState(const QByteArray& state);
};

}  // namespace app::ui::workspace
