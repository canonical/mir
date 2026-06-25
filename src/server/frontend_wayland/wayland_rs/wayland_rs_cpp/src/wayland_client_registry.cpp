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

#include "wayland_client_registry.h"
#include "wayland_rs/src/ffi.rs.h"

#include <algorithm>

namespace mwr = mir::wayland_rs;

void mwr::WaylandClientRegistry::add_client(std::shared_ptr<Client> const& client)
{
    clients.push_back(client);
}

void mwr::WaylandClientRegistry::remove_client(RawWlClientId const& id)
{
    std::erase_if(clients, [&](std::shared_ptr<Client> const& client)
        {
            return id->equals(client->raw_client()->id());
        });
}

auto mwr::WaylandClientRegistry::from(RawWlClient const& client) const -> std::shared_ptr<Client>
{
    return from(client->id());
}

auto mwr::WaylandClientRegistry::from(RawWlClientId const& id) const -> std::shared_ptr<Client>
{
    auto const it = std::find_if(
        clients.begin(),
        clients.end(),
        [&](std::shared_ptr<Client> const& client)
        {
            return id->equals(client->raw_client()->id());
        });

    return it != clients.end() ? *it : nullptr;
}
