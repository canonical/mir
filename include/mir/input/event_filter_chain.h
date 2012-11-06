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

#ifndef MIR_INPUT_EVENT_FILTER_CHAIN_H_
#define MIR_INPUT_EVENT_FILTER_CHAIN_H_

#include <memory>
#include <vector>

#include "mir/input/event_filter.h"

namespace android
{
    class InputEvent;
}

namespace mir
{
namespace input
{

class EventFilterChain : public EventFilter
{
public:
    explicit EventFilterChain() {}
    virtual ~EventFilterChain() {}

    virtual bool filter_event(const android::InputEvent *event);
    virtual void add_filter(std::shared_ptr<EventFilter> const& filter);

protected:
    EventFilterChain(const EventFilterChain&) = delete;
    EventFilterChain& operator=(const EventFilterChain&) = delete;

private:
    typedef std::vector<std::shared_ptr<EventFilter>> EventFilterVector;
    EventFilterVector filters;
};

}
}

#endif // MIR_INPUT_EVENT_FILTER_H_
