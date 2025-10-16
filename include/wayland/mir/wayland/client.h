/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_WAYLAND_CLIENT_H_
#define MIR_WAYLAND_CLIENT_H_

struct wl_client;

#include <memory>
#include <optional>

#include <mir/wayland/lifetime_tracker.h>

class MirEvent;

namespace mir
{
namespace scene
{
class Session;
}

namespace wayland
{
class Resource;

class Client : public wayland::LifetimeTracker
{
public:
    /// Returns the Client object for the given libwayland client
    static auto from(wl_client* client) -> Client&;

    /// The underlying Wayland client
    virtual auto raw_client() const -> wl_client* = 0;

    /// True if the client's destroy listener has fired. The client object continues to exist after this until all
    /// resources have been cleaned up.
    virtual auto is_being_destroyed() const -> bool = 0;

    /// The Mir session associated with this client. Be careful when using this that it's actually the session you want.
    /// All clients have a session but the surfaces they create may get associated with additional sessions.
    ///
    /// For example all surfaces from a single XWayland server are attached to a single Client with a single cleint
    /// session, but their scene::Surfaces are associated with multiple sessions created in the XWayland frontend for
    /// individual apps.
    virtual auto client_session() const -> std::shared_ptr<scene::Session> = 0;

    /// Generate a new serial, and keep track of it attached to the given event. Event may be null.
    virtual auto next_serial(std::shared_ptr<MirEvent const> event) -> uint32_t = 0;

    /// Returns the event associated with the given serial. Returns optional{nullptr} if the serial is known, but not
    /// associated with an event. Returns nullopt if the serial is unkown/invalid.
    virtual auto event_for(uint32_t serial) -> std::optional<std::shared_ptr<MirEvent const>> = 0;

    /// The XDG output protocol implementation sends the Mir-internal logical position and scale of outputs multiplied
    /// by this. It's currently just used by XWayland.
    /// @{
    virtual void set_output_geometry_scale(float scale) = 0;
    virtual auto output_geometry_scale() -> float = 0;
    /// @}

protected:
    static void register_client(wl_client* raw, std::shared_ptr<Client> const& shared);
    static void unregister_client(wl_client* raw);

private:
    friend Resource;
    static auto shared_from(wl_client* client) -> std::shared_ptr<Client>;
};
}
}

#endif // MIR_WAYLAND_CLIENT_H_
