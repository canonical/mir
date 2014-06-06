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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "mir_event_distributor.h"

MirEventDistributor::MirEventDistributor() :
    next_fn_id{0},
    in_event{false}
{
}

int MirEventDistributor::register_event_handler(std::function<void(MirEvent const&)> const& fn)
{
    std::unique_lock<std::mutex> lock(mutex);

    int id = ++next_fn_id;
    event_handlers[id] = fn;
    return id;
}

void MirEventDistributor::unregister_event_handler(int id)
{
    std::lock_guard<std::recursive_mutex> thread_lock(thread_mutex);
    std::unique_lock<std::mutex> lock(mutex);

    if (in_event)
        delete_later_ids.insert(id);
    else
        event_handlers.erase(id);
}

void MirEventDistributor::handle_event(MirEvent const& event)
{
    std::lock_guard<std::recursive_mutex> thread_lock(thread_mutex);
    std::unique_lock<std::mutex> lock(mutex);

    auto event_handlers_copy(event_handlers);

    for (auto const& handler : event_handlers_copy)
    {
        if (delete_later_ids.find(handler.first) == delete_later_ids.end())
        {
            in_event = true;

            lock.unlock();
            handler.second(event);
            lock.lock();

            in_event = false;
        }
    }

    for (int id : delete_later_ids)
        event_handlers.erase(id);

    delete_later_ids.clear();
}
