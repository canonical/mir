/*
 * Copyright © 2022 Canonical Ltd.
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

#include "mir/wayland/client.h"
#include "mir/synchronised.h"
#include "mir/fatal.h"

#include <vector>
#include <algorithm>

namespace mw = mir::wayland;

namespace
{
/// All operations for the same display should happen on the same thread, but since in theory a single process could
/// manage multiple Wayland displays, best to keep global state threadsafe.
mir::Synchronised<std::vector<std::pair<wl_client*, mw::Client*>>> client_map;
}

auto mw::Client::from(wl_client* client) -> Client&
{
    auto const map = client_map.lock();
    for (auto const& pair : *map)
    {
        if (pair.first == client)
        {
            return *pair.second;
        }
    }
    fatal_error("Client::from(): wl_client %p is %s", static_cast<void*>(client), client ? "unknown" : "null");
    abort(); // Make compiler happy
}

void mw::Client::register_client(Client* client)
{
    auto const map = client_map.lock();
    map->push_back({client->raw_client(), client});
}

void mw::Client::unregister_client(Client* client)
{
    auto const map = client_map.lock();
    map->erase(remove_if(
            begin(*map),
            end(*map),
            [&](auto& item){ return item.second == client; }),
        end(*map));
}
