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

#ifndef MIR_FRONTEND_WAYLAND_CLIENT_NOTIFIER_H_
#define MIR_FRONTEND_WAYLAND_CLIENT_NOTIFIER_H_

#include "wayland_client.h"
#include "wayland_server_notification_handler.h"

#include <functional>
#include <memory>

namespace mir
{
namespace scene
{
class Session;
}
namespace shell
{
class Shell;
}

namespace wayland_rs
{
class WaylandClientRegistry;
}

namespace frontend
{
class SessionAuthorizer;

/// Bridges the Rust server's client lifecycle notifications onto Mir.
///
/// On connect it authorizes the client, opens a `scene::Session`, wraps the raw
/// Rust client in a `WaylandClient`, registers it, and notifies any pending
/// per-fd connect handler. On disconnect it closes the session and removes the
/// client from the registry.
///
/// All callbacks run on the Wayland event-loop thread.
class WaylandClientNotifier : public wayland_rs::WaylandServerNotificationHandler
{
public:
    WaylandClientNotifier(
        std::shared_ptr<shell::Shell> shell,
        std::shared_ptr<SessionAuthorizer> session_authorizer,
        wayland_rs::WaylandClientRegistry& registry,
        WaylandSerialSource serial_source,
        std::function<void(int socket_fd, std::shared_ptr<scene::Session> const& session)> on_client_connected);

    auto client_added(rust::Box<wayland_rs::WaylandClient> wayland_client) -> void override;
    auto client_removed(rust::Box<wayland_rs::WaylandClientId> id) -> void override;

private:
    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<SessionAuthorizer> const session_authorizer;
    wayland_rs::WaylandClientRegistry& registry;
    WaylandSerialSource const serial_source;
    std::function<void(int socket_fd, std::shared_ptr<scene::Session> const& session)> const on_client_connected;
};
}
}

#endif // MIR_FRONTEND_WAYLAND_CLIENT_NOTIFIER_H_
