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

#include "wl_client.h"

#include "null_event_sink.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/frontend/session_credentials.h"
#include "mir/shell/shell.h"
#include "mir/scene/session.h"

#include <wayland-server-core.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

namespace
{
/// The context required for creating new WlClient's from wl_client*s
struct ConstructionCtx
{
    ConstructionCtx(
        std::shared_ptr<msh::Shell> const& shell,
        std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
        std::function<void(mf::WlClient&)>&& client_created_callback)
        : shell{shell},
          session_authorizer{session_authorizer},
          client_created_callback{std::make_unique<std::function<void(mf::WlClient&)>>(std::move(client_created_callback))}
    {
    }

    wl_listener client_construction_listener;
    wl_listener display_destruction_listener;
    std::shared_ptr<msh::Shell> const shell;
    std::shared_ptr<mf::SessionAuthorizer> const session_authorizer;
    /// Needs to be a pointer so std::is_standard_layout passes
    std::unique_ptr<std::function<void(mf::WlClient&)>> const client_created_callback;
};

static_assert(
    std::is_standard_layout<ConstructionCtx>::value,
    "ConstructionCtx must be standard layout for wl_container_of to be defined behaviour.");

/// Intermediary between the wl_client* and WlClient
struct ClientCtx
{
    ClientCtx(std::unique_ptr<mf::WlClient>&& client)
        : client{std::move(client)}
    {
    }

    static auto from(wl_listener* listener) -> ClientCtx*
    {
        ClientCtx* ctx;
        ctx = wl_container_of(listener, ctx, destroy_listener);
        return ctx;
    }

    std::unique_ptr<mf::WlClient> const client;
    wl_listener destroy_listener;
};

static_assert(
    std::is_standard_layout<ClientCtx>::value,
    "ClientCtx must be standard layout for wl_container_of to be defined behaviour");

void cleanup_construction_ctx(wl_listener* listener, void*)
{
    ConstructionCtx* construction_context;
    construction_context = wl_container_of(listener, construction_context, display_destruction_listener);
    delete construction_context;
}

void cleanup_client_ctx(wl_listener* listener, void* /*data*/)
{
    auto const ctx = ClientCtx::from(listener);
    // This ensures that further calls to wl_client_get_destroy_listener(client, &cleanup_client_ctx) - and hence
    // WlClient::from(client) - return nullptr.
    wl_list_remove(&ctx->destroy_listener.link);
    delete ctx;
}
}

void mf::WlClient::setup_new_client_handler(
    wl_display* display,
    std::shared_ptr<shell::Shell> const& shell,
    std::shared_ptr<SessionAuthorizer> const& session_authorizer,
    std::function<void(WlClient&)>&& client_created_callback)
{
    auto context = new ConstructionCtx{shell, session_authorizer, std::move(client_created_callback)};

    context->client_construction_listener.notify = &handle_client_created;
    wl_display_add_client_created_listener(display, &context->client_construction_listener);

    // This handles deleting the ConstructionCtx we just created when the display is destoryed
    context->display_destruction_listener.notify = &cleanup_construction_ctx;
    wl_display_add_destroy_listener(display, &context->display_destruction_listener);
}

auto mf::WlClient::from(wl_client* client) -> WlClient*
{
    if (!client)
    {
        return nullptr;
    }
    auto listener = wl_client_get_destroy_listener(client, &cleanup_client_ctx);
    auto ctx = ClientCtx::from(listener);
    return ctx ? ctx->client.get() : nullptr;
}

mf::WlClient::~WlClient()
{
    shell->close_session(session);
}

mf::WlClient::WlClient(wl_client* client, std::shared_ptr<ms::Session> const& session, msh::Shell* shell)
    : shell{shell},
      client{client},
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
        "",
        std::make_shared<mf::NullEventSink>());

    // Can't use std::make_unique because WlClient constructor is private
    auto wl_client = std::unique_ptr<mf::WlClient>{
        new mf::WlClient{client, session, construction_context->shell.get()}};
    auto client_context = new ClientCtx{std::move(wl_client)};
    client_context->destroy_listener.notify = &cleanup_client_ctx;
    wl_client_add_destroy_listener(client, &client_context->destroy_listener);

    (*construction_context->client_created_callback)(*client_context->client.get());
}
