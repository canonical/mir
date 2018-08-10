/*
 * Copyright © 2012 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "event_filter_chain_dispatcher.h"

namespace mi = mir::input;

mi::EventFilterChainDispatcher::EventFilterChainDispatcher(
    std::vector<std::weak_ptr<mi::EventFilter>> initial_filters,
    std::shared_ptr<mi::InputDispatcher> const& next_dispatcher)
    : filters(std::move(initial_filters)),
      next_dispatcher(next_dispatcher)
{
}

// TODO: It probably makes sense to provide keymapped events.
bool mi::EventFilterChainDispatcher::handle(MirEvent const& event)
{
    std::lock_guard<std::mutex> lg(filter_guard);
    
    auto it = filters.begin();
    while (it != filters.end())
    {
        auto filter = (*it).lock();
        if (!filter)
        {
            it = filters.erase(it);
            continue;
        }
        if (filter->handle(event)) return true;
        ++it;
    }
    return false;
}

void mi::EventFilterChainDispatcher::append(std::weak_ptr<EventFilter> const& filter)
{
    std::lock_guard<std::mutex> lg(filter_guard);

    filters.push_back(filter);
}

void mi::EventFilterChainDispatcher::prepend(std::weak_ptr<EventFilter> const& filter)
{
    std::lock_guard<std::mutex> lg(filter_guard);
        
    filters.insert(filters.begin(), filter);
}

bool mi::EventFilterChainDispatcher::dispatch(std::shared_ptr<MirEvent const> const& event)
{
    if (!handle(*event))
        return next_dispatcher->dispatch(event);
    return true;
}

// Should we start/stop dispatch of filter chain here?
// Why would we want to?
void mi::EventFilterChainDispatcher::start()
{
    next_dispatcher->start();
}

void mi::EventFilterChainDispatcher::stop()
{
     next_dispatcher->stop();
}
