/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */
#ifndef MIR_EVENT_FILTER_DISPATCHER_POLICY_H_
#define MIR_EVENT_FILTER_DISPATCHER_POLICY_H_

#include "dummy_input_dispatcher_policy.h"
#include "mir/input/event_filter.h"

namespace mir
{
namespace input
{
namespace android
{
//class EventFilter;

class EventFilterDispatcherPolicy : public DummyInputDispatcherPolicy
{
public:
    EventFilterDispatcherPolicy(std::shared_ptr<EventFilter> const& event_filter);
    virtual ~EventFilterDispatcherPolicy() {}

    virtual bool filterInputEvent(const droidinput::InputEvent* input_event,
                          uint32_t policy_flags);
    virtual void interceptKeyBeforeQueueing(const droidinput::KeyEvent* key_event,
                                    uint32_t& policy_flags);
protected:
    EventFilterDispatcherPolicy(const EventFilterDispatcherPolicy&) = delete;
    EventFilterDispatcherPolicy& operator=(const EventFilterDispatcherPolicy&) = delete;
private:
    std::shared_ptr<EventFilter> event_filter;
};

}
}
}

#endif // MIR_DUMMY_INPUT_DISPATCHER_POLICY_H_
