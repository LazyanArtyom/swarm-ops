#pragma once

#include <QLatin1StringView>

namespace app::commands::command_ids {

inline constexpr QLatin1StringView kOpen{"app.open"};
inline constexpr QLatin1StringView kSave{"app.save"};
inline constexpr QLatin1StringView kSaveAs{"app.save_as"};

}  // namespace app::commands::command_ids
