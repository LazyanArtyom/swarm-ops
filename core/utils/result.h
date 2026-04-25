#pragma once

#include <expected>

namespace app {

template <typename Value, typename Error>
using Result = std::expected<Value, Error>;

template <typename Error>
using VoidResult = std::expected<void, Error>;

}  // namespace app
