#include "workspace/page_lifecycle.h"

namespace app::ui::workspace {

void IPageLifecycle::OnActivate() {}

void IPageLifecycle::OnDeactivate() {}

bool IPageLifecycle::CanClose() const {
    return true;
}

QByteArray IPageLifecycle::SaveState() const {
    return {};
}

void IPageLifecycle::RestoreState(const QByteArray& /*state*/) {}

}  // namespace app::ui::workspace
