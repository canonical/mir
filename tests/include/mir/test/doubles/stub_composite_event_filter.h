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

#ifndef MIR_TEST_DOUBLES_STUB_COMPOSITE_EVENT_FILTER_H
#define MIR_TEST_DOUBLES_STUB_COMPOSITE_EVENT_FILTER_H

#include "mir/input/composite_event_filter.h"
#include <vector>

namespace mir::test::doubles
{
struct StubCompositeEventFilter : input::CompositeEventFilter
{
    bool handle(const MirEvent& event) override
    {
        for (auto const& filter : filters)
        {
            if (auto const locked = filter.lock())
            {
                if (locked->handle(event))
                    return true;
            }
        }

        return false;
    }

    void append(const std::weak_ptr<EventFilter>& filter) override
    {
        filters.push_back(filter);
    }

    void prepend(const std::weak_ptr<EventFilter>& filter) override
    {
        filters.insert(filters.begin(), filter);
    }

private:
    std::vector<std::weak_ptr<EventFilter>> filters;
};
}

#endif //MIR_TEST_DOUBLES_STUB_COMPOSITE_EVENT_FILTER_H
