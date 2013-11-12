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

#ifndef MIR_CLIENT_CONNECTION_CONFIGURATION_H_
#define MIR_CLIENT_CONNECTION_CONFIGURATION_H_

#include <memory>

namespace mir
{

namespace input
{
namespace receiver
{
class InputPlatform;
}
}

namespace logging
{
class Logger;
}

namespace client
{

namespace rpc
{
class MirBasicRpcChannel;
}

class SurfaceMap;
class Logger;
class ClientPlatformFactory;
class DisplayConfiguration;
class LifecycleControl;

class ConnectionConfiguration
{
public:
    virtual ~ConnectionConfiguration() = default;

    virtual std::shared_ptr<SurfaceMap> the_surface_map() = 0;
    virtual std::shared_ptr<rpc::MirBasicRpcChannel> the_rpc_channel() = 0;
    virtual std::shared_ptr<mir::logging::Logger> the_logger() = 0;
    virtual std::shared_ptr<ClientPlatformFactory> the_client_platform_factory() = 0;
    virtual std::shared_ptr<input::receiver::InputPlatform> the_input_platform() = 0;
    virtual std::shared_ptr<DisplayConfiguration> the_display_configuration() = 0;
    virtual std::shared_ptr<LifecycleControl> the_lifecycle_control() = 0;

protected:
    ConnectionConfiguration() = default;
    ConnectionConfiguration(ConnectionConfiguration const&) = delete;
    ConnectionConfiguration& operator=(ConnectionConfiguration const&) = delete;
};

}
}

#endif /* MIR_CLIENT_CONNECTION_CONFIGURATION_H_ */
