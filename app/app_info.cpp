#include "app/app_info.h"

namespace app {

QString AppInfo::WindowTitle() const {
    if (version.isEmpty()) {
        return display_name;
    }

    return QStringLiteral("%1 v%2").arg(display_name, version);
}

AppInfo BuildAppInfo() {
    return {
        .display_name = QStringLiteral(APP_DISPLAY_NAME),
        .slug = QStringLiteral(APP_SLUG),
        .id = QStringLiteral(APP_ID),
        .vendor = QStringLiteral(APP_VENDOR),
        .version = QStringLiteral(APP_VERSION),
        .release_date = QStringLiteral(APP_RELEASE_DATE),
    };
}

}  // namespace app
