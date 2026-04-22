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

#include "wl_client_registry.h"
#include "wl_client.h"
#include "wayland_rs/src/ffi.rs.h"
#include <mir/shell/shell.h>
#include <mir/frontend/session_authorizer.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mw = mir::wayland_rs;

mf::WlClientRegistry::WlClientRegistry(
    std::shared_ptr<shell::Shell> const& shell,
    std::shared_ptr<SessionAuthorizer> const& session_authorizer)
    : shell(shell), session_authorizer(session_authorizer)
{}

mf::WlClient* mf::WlClientRegistry::add_client(RawWlClient client)
{
    auto session = shell->open_session(
        client->pid(),
        Fd{IntOwnedFd{client->socket_fd()}},
        client->name().c_str()
    );
    auto const wl_client = std::make_shared<WlClient>(std::move(client), session, shell.get());
    clients.push_back(wl_client);
    return wl_client.get();
}

void mf::WlClientRegistry::delete_client(RawWlClientId client_id)
{
    std::erase_if(clients, [&](std::shared_ptr<WlClient> const& client)
    {
        return client_id->equals(client->raw_client()->id());
    });
}

std::shared_ptr<mir::frontend::WlClient> mf::WlClientRegistry::from(RawWlClient const& raw_client)
{
    auto const it = std::find_if(clients.begin(), clients.end(), [&](auto const& client)
    {
        return raw_client->id()->equals(client->raw_client()->id());
    });

    if (it != clients.end())
        return *it;

    return nullptr;
}
