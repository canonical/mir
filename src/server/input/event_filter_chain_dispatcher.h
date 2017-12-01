/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_INPUT_EVENT_FILTER_CHAIN_DISPATCHER_H_
#define MIR_INPUT_EVENT_FILTER_CHAIN_DISPATCHER_H_

#include "mir/input/composite_event_filter.h"
#include "mir/input/input_dispatcher.h"

#include <vector>
#include <mutex>

namespace mir
{
namespace input
{

class EventFilterChainDispatcher : public CompositeEventFilter, public mir::input::InputDispatcher
{
public:
    EventFilterChainDispatcher(
        std::initializer_list<std::shared_ptr<EventFilter> const> const& values,
        std::shared_ptr<InputDispatcher> const& next_dispatcher);

    // CompositeEventFilter
    bool handle(MirEvent const& event) override;
    void append(std::shared_ptr<EventFilter> const& filter) override;
    void prepend(std::shared_ptr<EventFilter> const& filter) override;

    // InputDispatcher
    bool dispatch(std::shared_ptr<MirEvent const> const& event) override;
    void start() override;
    void stop() override;
    
private:
    std::mutex filter_guard;
    
    std::vector<std::weak_ptr<EventFilter>> filters;
    std::shared_ptr<InputDispatcher> const next_dispatcher;
};

}
}

#endif // MIR_INPUT_EVENT_FILTER_CHAIN_DISPATCHER_H_
