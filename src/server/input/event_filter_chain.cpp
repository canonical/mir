/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir/input/event_filter_chain.h"

namespace mi = mir::input;

mi::EventFilterChain::EventFilterChain(
    std::initializer_list<std::shared_ptr<mi::EventFilter> const> const& values)
    : filters(values.begin(), values.end())
{
}

bool mi::EventFilterChain::handle(MirEvent const& event)
{
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

void mi::EventFilterChain::append(std::shared_ptr<EventFilter> const& filter)
{
    filters.push_back(filter);
}

void mi::EventFilterChain::prepend(std::shared_ptr<EventFilter> const& filter)
{
    filters.insert(filters.begin(), filter);
}
