/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_WAYLANDRS_DYNAMIC_PROTOCOL
#define MIR_WAYLANDRS_DYNAMIC_PROTOCOL

#include <rust/cxx.h>

#include <cstdint>
#include <memory>
#include <stdexcept>

namespace mir
{
namespace wayland
{
struct DynamicArgument;
struct DynamicResource;
struct WaylandClient;
struct WaylandClientId;

/// Handles the requests of one object of a protocol loaded at runtime (with
/// `load_dynamic_protocol`).
///
/// All calls happen on the Wayland event-loop thread. Throwing
/// `mir::wayland::ProtocolError` posts that protocol error to the client; any
/// other exception is reported to the client as an internal server error.
class DynamicResourceHandler
{
public:
    virtual ~DynamicResourceHandler() = default;

    /// A request that does not create an object.
    virtual void on_request(uint32_t opcode, rust::Vec<DynamicArgument> args) = 0;

    /// A request that creates `new_resource` (its `new_id` argument). Return
    /// the handler for the new object.
    virtual auto on_constructor_request(
        uint32_t /*opcode*/,
        rust::Vec<DynamicArgument> /*args*/,
        rust::Box<DynamicResource> /*new_resource*/) -> std::shared_ptr<DynamicResourceHandler>
    {
        throw std::logic_error{"Request creating an object is not implemented"};
    }

    /// The object has been destroyed, by a destructor request, by the server or
    /// because its client disconnected. No further calls follow.
    virtual void on_destroyed() {}

protected:
    DynamicResourceHandler() = default;
    DynamicResourceHandler(DynamicResourceHandler const&) = delete;
    auto operator=(DynamicResourceHandler const&) -> DynamicResourceHandler& = delete;
};

/// Handles a global of a protocol loaded at runtime (see
/// `WaylandServer::create_dynamic_global`).
///
/// All calls happen on the Wayland event-loop thread.
class DynamicGlobalHandler
{
public:
    virtual ~DynamicGlobalHandler() = default;

    /// `client` has bound the global, creating `resource`. Return its handler.
    virtual auto bind(rust::Box<WaylandClient> client, rust::Box<DynamicResource> resource)
        -> std::shared_ptr<DynamicResourceHandler> = 0;

    /// Whether the global is advertised to the client.
    virtual auto can_view(rust::Box<WaylandClientId> /*client_id*/) -> bool { return true; }

protected:
    DynamicGlobalHandler() = default;
    DynamicGlobalHandler(DynamicGlobalHandler const&) = delete;
    auto operator=(DynamicGlobalHandler const&) -> DynamicGlobalHandler& = delete;
};
}
}

#endif // MIR_WAYLANDRS_DYNAMIC_PROTOCOL
