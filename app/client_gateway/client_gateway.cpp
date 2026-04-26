#include "app/client_gateway/client_gateway.h"

#if defined(SWARMOPS_CLIENT_GATEWAY_SIMULATOR) && SWARMOPS_CLIENT_GATEWAY_SIMULATOR
#include "app/client_gateway/simulated_client_gateway.h"
#else
#include "app/client_gateway/api_gateway_client.h"
#endif

namespace app::client_gateway {

IClientGateway& ClientGatewayRuntime() {
#if defined(SWARMOPS_CLIENT_GATEWAY_SIMULATOR) && SWARMOPS_CLIENT_GATEWAY_SIMULATOR
    static auto* gateway = new SimulatedClientGateway();
#else
    static auto* gateway = new ApiGatewayClient();
#endif
    return *gateway;
}

}  // namespace app::client_gateway
