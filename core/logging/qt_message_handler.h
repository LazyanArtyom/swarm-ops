#pragma once

namespace app::logging {

class QtMessageHandlerBridge final {
   public:
    QtMessageHandlerBridge() = delete;
    QtMessageHandlerBridge(const QtMessageHandlerBridge&) = delete;
    QtMessageHandlerBridge& operator=(const QtMessageHandlerBridge&) = delete;

    static void Install();
    static void Uninstall();
};

}  // namespace app::logging
