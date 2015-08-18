/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_CLIENT_DEFAULT_CONNECTION_CONFIGURATION_H_
#define MIR_CLIENT_DEFAULT_CONNECTION_CONFIGURATION_H_

#include "connection_configuration.h"

#include "mir/cached_ptr.h"

#include <string>

namespace mir
{
class SharedLibraryProberReport;

namespace input
{
namespace receiver
{
class InputReceiverReport;
}
}
namespace client
{
class EventDistributor;

namespace rpc
{
class RpcReport;
}

class DefaultConnectionConfiguration : public ConnectionConfiguration
{
public:
    DefaultConnectionConfiguration(std::string const& socket_file);

    std::shared_ptr<ConnectionSurfaceMap> the_surface_map() override;
    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> the_rpc_channel() override;
    std::shared_ptr<mir::logging::Logger> the_logger() override;
    std::shared_ptr<ClientPlatformFactory> the_client_platform_factory() override;
    std::shared_ptr<input::receiver::InputPlatform> the_input_platform() override;
    std::shared_ptr<DisplayConfiguration> the_display_configuration() override;
    std::shared_ptr<LifecycleControl> the_lifecycle_control() override;
    std::shared_ptr<PingHandler> the_ping_handler() override;
    std::shared_ptr<EventSink> the_event_sink() override;
    std::shared_ptr<EventHandlerRegister> the_event_handler_register() override;
    std::shared_ptr<mir::SharedLibraryProberReport> the_shared_library_prober_report();

    virtual std::string the_socket_file();
    virtual std::shared_ptr<rpc::RpcReport> the_rpc_report();
    virtual std::shared_ptr<input::receiver::InputReceiverReport> the_input_receiver_report();

protected:
    CachedPtr<mir::client::rpc::MirBasicRpcChannel> rpc_channel;
    CachedPtr<mir::logging::Logger> logger;
    CachedPtr<ClientPlatformFactory> client_platform_factory;
    CachedPtr<input::receiver::InputPlatform> input_platform;
    CachedPtr<ConnectionSurfaceMap> surface_map;
    CachedPtr<DisplayConfiguration> display_configuration;
    CachedPtr<LifecycleControl> lifecycle_control;
    CachedPtr<PingHandler> ping_handler;
    CachedPtr<EventDistributor> event_distributor;

    CachedPtr<rpc::RpcReport> rpc_report;
    CachedPtr<input::receiver::InputReceiverReport> input_receiver_report;
    CachedPtr<mir::SharedLibraryProberReport>  shared_library_prober_report;

private:
    std::string const socket_file;
};

}
}

#endif /* MIR_CLIENT_DEFAULT_CONNECTION_CONFIGURATION_H_ */
