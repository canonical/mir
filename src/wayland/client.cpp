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

#include "mir/wayland/client.h"
#include "mir/synchronised.h"
#include "mir/fatal.h"

#include <vector>
#include <algorithm>

namespace mw = mir::wayland;

namespace
{
/// All operations for the same display should happen on the same thread, but since in theory a single process could
/// manage multiple Wayland targets, best to keep global state threadsafe.
mir::Synchronised<std::vector<std::pair<wl_client*, std::weak_ptr<mw::Client>>>> client_map;
}

auto mw::Client::from(wl_client* client) -> Client&
{
    return *shared_from(client);
}

void mw::Client::register_client(wl_client* raw, std::shared_ptr<Client> const& shared)
{
    client_map.lock()->push_back({raw, shared});
}

void mw::Client::unregister_client(wl_client* raw)
{
    auto const map = client_map.lock();
    map->erase(remove_if(
            begin(*map),
            end(*map),
            [&](auto& item){ return item.first == raw; }),
        end(*map));
}

auto mw::Client::shared_from(wl_client* client) -> std::shared_ptr<Client>
{
    auto const locked = client_map.lock();
    for (auto& info : *locked)
    {
        if (info.first == client)
        {
            if (auto const shared = info.second.lock())
            {
                return shared;
            }
            else
            {
                // The client should remove itself from the map in it's destructor and should be destroyed/accessed on a
                // single thread, so this should never happen
                mir::fatal_error("wl_client %p expired");
            }
        }
    }
    mir::fatal_error("wl_client %p is %s", static_cast<void*>(client), client ? "unknown" : "null");
    abort(); // Make compiler happy
}
