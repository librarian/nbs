#pragma once

#include "public.h"

#include <cloud/blockstore/libs/diagnostics/public.h>

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/common/startable.h>

namespace NCloud::NBlockStore::NServer {

////////////////////////////////////////////////////////////////////////////////

struct IEndpointProxyServer: public IStartable
{
};

////////////////////////////////////////////////////////////////////////////////

struct TEndpointProxyServerConfig
{
    ui16 Port;
    ui16 SecurePort;
    TString RootCertsFile;
    TString KeyFile;
    TString CertFile;

    TEndpointProxyServerConfig(
            ui16 port,
            ui16 securePort,
            TString rootCertsFile,
            TString keyFile,
            TString certFile)
        : Port(port)
        , SecurePort(securePort)
        , RootCertsFile(std::move(rootCertsFile))
        , KeyFile(std::move(keyFile))
        , CertFile(std::move(certFile))
    {
    }
};

////////////////////////////////////////////////////////////////////////////////

IEndpointProxyServerPtr CreateServer(
    TEndpointProxyServerConfig config,
    ITimerPtr timer,
    ISchedulerPtr scheduler,
    ILoggingServicePtr logging);

}   // namespace NCloud::NBlockStore::NServer
