/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_WL_CLIENT_H_
#define MIR_FRONTEND_WL_CLIENT_H_

struct wl_client;
struct wl_listener;
struct wl_display;

#include <memory>
#include <functional>

#include "mir/wayland/wayland_base.h"

namespace mir
{
namespace shell
{
class Shell;
}
namespace scene
{
class Session;
}

namespace frontend
{
class SessionAuthorizer;

class WlClient : public wayland::LifetimeTracker
{
public:
    /// Initializes a ConstructionCtx that will create a WlClient for each wl_client created on the display. Should only
    /// be called once per display. Destruction of the ConstructionCtx is handled automatically.
    static void setup_new_client_handler(
        wl_display* display,
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<SessionAuthorizer> const& session_authorizer,
        std::function<void(WlClient&)>&& client_created_callback);

    static auto from(wl_client* client) -> WlClient*;

    ~WlClient();

    /// The underlying Wayland client
    auto raw_client() const -> wl_client* { return client; }

    /// The Mir session associated with this client. Be careful when using this that it's actually the session you want.
    /// All clients have a session but the surfaces they create may get associated with additional sessions.
    ///
    /// For example all surfaces from a single XWayland server are attached to a single WlClient with a single cleint
    /// session, but their scene::Surfaces are associated with multiple sessions created in the XWayland frontend for
    /// individual apps.
    auto client_session() const -> std::shared_ptr<scene::Session> { return session; }

    /// The XDG output protocol implementation sends the Mir-internal logical position and scale of outputs multiplied
    /// by this. It's currently just used by XWayland.
    /// @{
    auto set_output_geometry_scale(float scale) { output_geometry_scale_ = scale; }
    auto output_geometry_scale() -> float { return output_geometry_scale_; }
    /// @}

private:
    WlClient(wl_client* client, std::shared_ptr<scene::Session> const& session, shell::Shell* shell);

    static void handle_client_created(wl_listener* listener, void* data);

    /// This shell is owned by the ClientSessionConstructor, which outlives all clients.
    shell::Shell* const shell;
    wl_client* const client;
    std::shared_ptr<scene::Session> const session;

    float output_geometry_scale_{1};
};
}
}

#endif // MIR_FRONTEND_WL_CLIENT_H_
