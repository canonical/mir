/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "mir_event_distributor.h"

MirEventDistributor::MirEventDistributor() :
    next_fn_id{0}
{
}

int MirEventDistributor::register_event_handler(std::function<void(MirEvent const&)> const& fn)
{
    std::unique_lock<decltype(mutex)> lock(mutex);

    int id = ++next_fn_id;
    event_handlers[id] = fn;
    return id;
}

void MirEventDistributor::unregister_event_handler(int id)
{
    std::unique_lock<decltype(mutex)> lock(mutex);

    event_handlers.erase(id);
}

void MirEventDistributor::handle_event(MirEvent const& event)
{
    std::unique_lock<decltype(mutex)> lock(mutex);

    auto const event_handlers_copy(event_handlers);

    for (auto const& handler : event_handlers_copy)
    {
        // Ensure handler wasn't unregistered since making copy
        if (event_handlers.find(handler.first) != event_handlers.end())
        {
            handler.second(event);
        }
    }
}
