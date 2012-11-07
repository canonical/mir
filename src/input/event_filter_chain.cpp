/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

bool mi::EventFilterChain::handles(const android::InputEvent *event)
{
    for (auto it = filters.begin(); it != filters.end(); it++)
    {
        auto filter = *it;
        if (filter->handles(event)) return true;
    }
    return false;
}
 
void mi::EventFilterChain::add_filter(std::shared_ptr<mi::EventFilter> const& filter)
{
    filters.push_back(filter);
}

