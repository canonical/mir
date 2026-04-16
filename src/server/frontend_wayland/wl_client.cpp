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

#include "wl_client.h"
#include "std_layout_uptr.h"

#include <mir/frontend/session_authorizer.h>
#include <mir/frontend/session_credentials.h>
#include <mir/shell/shell.h>
#include <mir/scene/session.h>
#include <mir/fatal.h>

#include <wayland-server-core.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

namespace
{
constexpr int max_serial_event_pairs = 100;
}


mf::WlClient::~WlClient()
{
    shell->close_session(session);
    unregister_client(client);
}

auto mf::WlClient::next_serial(std::shared_ptr<MirEvent const> event) -> uint32_t
{
    auto const serial = wl_display_next_serial(display);
    serial_event_pairs.push_front({serial, event});
    while (serial_event_pairs.size() > max_serial_event_pairs)
    {
        serial_event_pairs.pop_back();
    }
    return serial;
}

auto mf::WlClient::event_for(uint32_t serial) -> std::optional<std::shared_ptr<MirEvent const>>
{
    for (auto const& pair : serial_event_pairs)
    {
        if (pair.first == serial)
        {
            return pair.second;
        }
    }
    return std::nullopt;
}

mf::WlClient::WlClient(rust::Box<wayland_rs::WaylandClient> client, std::shared_ptr<ms::Session> const& session, msh::Shell* shell)
    : shell{shell},
      client{std::move(client)},
      session{session}
{
}

void mf::WlClient::handle_client_created(wl_listener* listener, void* data)
{
    auto client = reinterpret_cast<wl_client*>(data);

    ConstructionCtx* construction_context;
    construction_context = wl_container_of(listener, construction_context, client_construction_listener);

    pid_t client_pid;
    uid_t client_uid;
    gid_t client_gid;
    wl_client_get_credentials(client, &client_pid, &client_uid, &client_gid);

    if (!construction_context->session_authorizer->connection_is_allowed({client_pid, client_uid, client_gid}))
    {
        wl_client_destroy(client);
        return;
    }

    auto session = construction_context->shell->open_session(
        client_pid,
        Fd{IntOwnedFd{wl_client_get_fd(client)}},
        "");

    // Can't use std::make_shared because WlClient constructor is private
    auto shared = std::shared_ptr<mf::WlClient>{
        new mf::WlClient{client, session, construction_context->shell.get()}};
    register_client(client, shared);

    auto client_context = new ClientCtx{shared.get(), {}};
    client_context->destroy_listener.notify = &handle_client_destroyed;
    wl_client_add_destroy_listener(client, &client_context->destroy_listener);

    client_context->client->owned_self = std::move(shared);

    (*construction_context->client_created_callback)(*client_context->client);
}

void mf::WlClient::handle_client_destroyed(wl_listener* listener, void* /*data*/)
{
    ClientCtx* ctx = wl_container_of(listener, ctx, destroy_listener);
    ctx->client->owned_self.reset();
    delete ctx;
}
