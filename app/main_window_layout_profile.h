#pragma once

#include <QString>
#include <cstdint>

namespace app {

enum class MainWindowLayoutProfileKind : std::uint8_t {
    kDefault,
    kCompact,
    kDebug,
    kProject,
};

struct SplitterWeights final {
    int navigation{1};
    int central{4};
    int info{1};
    int workspace{4};
    int console{1};
};

struct MainWindowLayoutProfile final {
    QString id;
    QString title;
    MainWindowLayoutProfileKind kind{MainWindowLayoutProfileKind::kDefault};
    bool navigation_visible{true};
    bool info_visible{true};
    bool console_visible{true};
    SplitterWeights weights;
};

namespace layout_profiles {

[[nodiscard]] MainWindowLayoutProfile Default();
[[nodiscard]] MainWindowLayoutProfile Compact();
[[nodiscard]] MainWindowLayoutProfile Debug();
[[nodiscard]] MainWindowLayoutProfile Project(QString profile_id, QString title);

}  // namespace layout_profiles

}  // namespace app
