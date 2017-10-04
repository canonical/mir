/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */
#ifndef MIR_CLIENT_RPC_MAKE_RPC_CHANNEL_H_
#define MIR_CLIENT_RPC_MAKE_RPC_CHANNEL_H_

#include "../lifecycle_control.h"
#include "../ping_handler.h"
#include "../error_handler.h"

#include <memory>

namespace mir
{
namespace input
{
class InputDevices;

namespace receiver
{
class InputReceiverReport;
}
}
namespace client
{
class SurfaceMap;
class DisplayConfiguration;
class EventSink;
class AsyncBufferFactory;

namespace rpc
{
class MirBasicRpcChannel;
class RpcReport;

std::shared_ptr<mir::client::rpc::MirBasicRpcChannel>
make_rpc_channel(
    std::string const& name,
    std::shared_ptr<SurfaceMap> const& map,
    std::shared_ptr<AsyncBufferFactory> const buffer_factory,
    std::shared_ptr<DisplayConfiguration> const& disp_conf,
    std::shared_ptr<input::InputDevices> const& input_devices,
    std::shared_ptr<RpcReport> const& rpc_report,
    std::shared_ptr<input::receiver::InputReceiverReport> const& input_report,
    std::shared_ptr<LifecycleControl> const& lifecycle_control,
    std::shared_ptr<PingHandler> const& ping_handler,
    std::shared_ptr<ErrorHandler> const& error_handler,
    std::shared_ptr<EventSink> const& event_distributor);
}
}
}

#endif /* MIR_CLIENT_RPC_MAKE_RPC_CHANNEL_H_ */
