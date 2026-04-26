#pragma once

#include <QObject>

namespace app::client_gateway {

class IMissionWorkspaceClient;
class IDroneGatewayClient;
class ISwarmRuntimeClient;

class IClientGateway : public QObject {
    Q_OBJECT

   public:
    using QObject::QObject;
    ~IClientGateway() override = default;

    [[nodiscard]] virtual IMissionWorkspaceClient& MissionWorkspaces() = 0;
    [[nodiscard]] virtual IDroneGatewayClient& DroneGateway() = 0;
    [[nodiscard]] virtual ISwarmRuntimeClient& SwarmRuntime() = 0;
};

[[nodiscard]] IClientGateway& ClientGatewayRuntime();

}  // namespace app::client_gateway
