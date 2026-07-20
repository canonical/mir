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

#include "wayland_client_notifier.h"

#include "wayland_rs/src/ffi.rs.h"
#include "wayland_client_registry.h"

#include <mir/fd.h>
#include <mir/frontend/session_authorizer.h>
#include <mir/frontend/session_credentials.h>
#include <mir/log.h>
#include <mir/scene/session.h>
#include <mir/shell/shell.h>

#include <unistd.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mwrs = mir::wayland_rs;

mf::WaylandClientNotifier::WaylandClientNotifier(
    std::shared_ptr<msh::Shell> shell,
    std::shared_ptr<SessionAuthorizer> session_authorizer,
    mwrs::WaylandClientRegistry& registry,
    WaylandSerialSource serial_source,
    std::function<void(int, std::shared_ptr<ms::Session> const&, std::shared_ptr<mwrs::Client> const&)> on_client_connected)
    : shell{std::move(shell)},
      session_authorizer{std::move(session_authorizer)},
      registry{registry},
      serial_source{std::move(serial_source)},
      on_client_connected{std::move(on_client_connected)}
{
}

auto mf::WaylandClientNotifier::client_added(rust::Box<mwrs::WaylandClient> wayland_client) -> void
try
{
    auto const pid = wayland_client->pid();
    auto const uid = wayland_client->uid();
    auto const gid = wayland_client->gid();
    auto const socket_fd = wayland_client->socket_fd();

    if (!session_authorizer->connection_is_allowed({pid, uid, gid}))
    {
        // The Rust backend does not yet expose a way to forcibly disconnect a
        // client from the notification handler, so we simply decline to open a
        // session and register the client. Without a registered Client no
        // globals resolve for it (see GlobalFactory::can_view), so it cannot
        // make requests.
        mir::log_info("Wayland client (pid %d) rejected by session authorizer", pid);
        return;
    }

    // The Rust client owns its socket, so hand the session a duplicate to own.
    auto session = shell->open_session(pid, Fd{IntOwnedFd{::dup(socket_fd)}}, "");

    auto client = std::make_shared<WaylandClient>(
        std::move(wayland_client),
        session,
        shell,
        serial_source);

    registry.add_client(client);

    if (on_client_connected)
    {
        on_client_connected(socket_fd, session, client);
    }
}
catch (rust::Error const& error)
{
    mir::log_warning("Failed to handle Wayland client connection: %s", error.what());
}

auto mf::WaylandClientNotifier::client_removed(rust::Box<mwrs::WaylandClientId> id) -> void
{
    if (auto const client = registry.from(id))
    {
        static_cast<WaylandClient&>(*client).mark_being_destroyed();
    }
    registry.remove_client(id);
}
