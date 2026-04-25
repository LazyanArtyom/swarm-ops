#include "app/main_window_layout_profile.h"

#include <utility>

namespace app::layout_profiles {
namespace {

constexpr int kHiddenPanelWeight = 0;
constexpr int kSidePanelWeight = 1;
constexpr int kDefaultCentralWeight = 4;
constexpr int kCompactCentralWeight = 5;
constexpr int kDebugCentralWeight = 3;
constexpr int kDefaultWorkspaceWeight = 4;
constexpr int kDebugWorkspaceWeight = 3;
constexpr int kDefaultConsoleWeight = 1;
constexpr int kDebugConsoleWeight = 2;

}  // namespace

MainWindowLayoutProfile Default() {
    return {
        .id = QStringLiteral("default"),
        .title = QStringLiteral("Default"),
        .kind = MainWindowLayoutProfileKind::kDefault,
        .navigation_visible = true,
        .info_visible = true,
        .console_visible = true,
        .weights =
            {
                .navigation = kSidePanelWeight,
                .central = kDefaultCentralWeight,
                .info = kSidePanelWeight,
                .workspace = kDefaultWorkspaceWeight,
                .console = kDefaultConsoleWeight,
            },
    };
}

MainWindowLayoutProfile Compact() {
    return {
        .id = QStringLiteral("compact"),
        .title = QStringLiteral("Compact"),
        .kind = MainWindowLayoutProfileKind::kCompact,
        .navigation_visible = true,
        .info_visible = false,
        .console_visible = false,
        .weights =
            {
                .navigation = kSidePanelWeight,
                .central = kCompactCentralWeight,
                .info = kHiddenPanelWeight,
                .workspace = kDefaultWorkspaceWeight,
                .console = kHiddenPanelWeight,
            },
    };
}

MainWindowLayoutProfile Debug() {
    return {
        .id = QStringLiteral("debug"),
        .title = QStringLiteral("Debug"),
        .kind = MainWindowLayoutProfileKind::kDebug,
        .navigation_visible = true,
        .info_visible = true,
        .console_visible = true,
        .weights =
            {
                .navigation = kSidePanelWeight,
                .central = kDebugCentralWeight,
                .info = kSidePanelWeight,
                .workspace = kDebugWorkspaceWeight,
                .console = kDebugConsoleWeight,
            },
    };
}

MainWindowLayoutProfile Project(QString profile_id, QString title) {
    MainWindowLayoutProfile profile = Default();
    profile.id = std::move(profile_id);
    profile.title = std::move(title);
    profile.kind = MainWindowLayoutProfileKind::kProject;
    return profile;
}

}  // namespace app::layout_profiles
