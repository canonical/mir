/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Nick Dedekind <nick.dedekind@gmail.com>
 */

#include "event_distributor.h"

namespace mcl = mir::client;

mcl::EventDistributor::EventDistributor() :
    next_fn_id(0)
{
}

int mcl::EventDistributor::register_event_handler(std::function<void(MirEvent const&)> const& fn)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    int id = next_id();
    event_handlers[id] = fn;
    return id;
}

void mcl::EventDistributor::unregister_event_handler(int id)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    event_handlers.erase(id);
}

void mcl::EventDistributor::handle_event(MirEvent const& event)
{
    std::lock_guard<decltype(mutex)> lock(mutex);

    for (auto const& fn : event_handlers)
    {
        fn.second(event);
    }
}

int mcl::EventDistributor::next_id()
{
    return ++next_fn_id;
}