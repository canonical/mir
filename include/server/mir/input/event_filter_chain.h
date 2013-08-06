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

#ifndef MIR_INPUT_EVENT_FILTER_CHAIN_H_
#define MIR_INPUT_EVENT_FILTER_CHAIN_H_

#include "mir/input/composite_event_filter.h"

#include <vector>

namespace mir
{
namespace input
{

class EventFilterChain : public CompositeEventFilter
{
public:
    explicit EventFilterChain(std::initializer_list<std::shared_ptr<EventFilter> const> const& values);

    bool handle(MirEvent const& event);
    void append(std::shared_ptr<EventFilter> const& filter);
    void prepend(std::shared_ptr<EventFilter> const& filter);

private:
    std::vector<std::weak_ptr<EventFilter>> filters;
};

}
}

#endif // MIR_INPUT_EVENT_FILTER_CHAIN_H_
