#pragma once

#include <QString>

namespace app {

struct AppInfo final {
    QString display_name;
    QString slug;
    QString id;
    QString vendor;
    QString version;
    QString release_date;

    [[nodiscard]] QString WindowTitle() const;
};

[[nodiscard]] AppInfo BuildAppInfo();

}  // namespace app
