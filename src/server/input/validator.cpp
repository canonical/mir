/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "mir/input/validator.h"
#include "mir/events/event_builders.h"
#include "mir_toolkit/event.h"

#include <set>
#include <algorithm>

namespace mi = mir::input;
namespace mev = mir::events;

namespace
{
std::vector<mev::ContactState> get_contact_state(MirTouchEvent const* event)
{
    std::vector<mev::ContactState> ret;
    ret.reserve(mir_touch_event_point_count(event));

    for (size_t i = 0, count = mir_touch_event_point_count(event); i != count; ++i)
    {
        ret.push_back(mev::ContactState{
                      mir_touch_event_id(event, i),
                      mir_touch_event_action(event, i),
                      mir_touch_event_tooltype(event, i),
                      mir_touch_event_axis_value(event, i, mir_touch_axis_x),
                      mir_touch_event_axis_value(event, i, mir_touch_axis_y),
                      mir_touch_event_axis_value(event, i, mir_touch_axis_pressure),
                      mir_touch_event_axis_value(event, i, mir_touch_axis_touch_major),
                      mir_touch_event_axis_value(event, i, mir_touch_axis_touch_minor),
                      0.0f
                      });
    }

    return ret;
}
}

mi::Validator::Validator(std::function<void(MirEvent const&)> const& dispatch_valid_event)
    : dispatch_valid_event(dispatch_valid_event)
{
}

// Currently we only validate touch events as they are the most problematic
void mi::Validator::validate_and_dispatch(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
    {
        dispatch_valid_event(event);
        return;
    }
    auto iev = mir_event_get_input_event(&event);
    if (mir_input_event_get_type(iev) != mir_input_event_type_touch)
    {
        dispatch_valid_event(event);
        return;
    }
    handle_touch_event(event);

    return;
}

// This is the core of the touch validator. Given a valid event 'last_ev' which was the last_event to be dispatched
// this function must dispatch events such that 'ev' is a valid event to dispatch.
// Our requirements for touch validity are simple:
//     1. A touch point (unique per ID) can not vanish without being released.
//     2. A touch point can not appear without coming down.
// Our algorithm to ensure a candidate event can be dispatched is as follows:
//
//     First we look at the last event to build a set of expected touch ID's. That is to say
// each touch point in the last event which did not have touch_action_up (signifying its dissapearance)
// is expected to be in the candidate event. Likewise we go through the candidate event can produce a set of events
// we have found.
//
//     We now check for expected events which were not found, e.g. touch points which are missing a release.
// For each of these touch points we can take the coordinates for these points from the last event
// and dispatch an event which releases the missing point.
//
//     Now we check for found touch points which were not expected. If these show up with mir_touch_action_down
// things are fine. On the other hand if they show up with mir_touch_action_change then a touch point
// has appeared before its gone down and thus we must inject an event signifying this touch going down.
void mi::Validator::ensure_stream_validity_locked(
    std::lock_guard<std::mutex> const&,
    std::vector<mev::ContactState> const& new_event,
    std::vector<mev::ContactState>& last_state,
    std::function<void(std::vector<mev::ContactState> const&)> const& dispatch)
{
    using TouchSet = std::set<MirTouchId>;
    TouchSet expected;
    for (auto const& contact : last_state)
        expected.insert(contact.touch_id);

    TouchSet in_next_event;
    TouchSet changed_in_next_event;
    for (auto const& contact : new_event)
    {
        in_next_event.insert(contact.touch_id);
        if (contact.action != mir_touch_action_down)
            changed_in_next_event.insert(contact.touch_id);
    }

    std::vector<MirTouchId> missing_up;
    set_difference(begin(expected), end(expected),
                   begin(in_next_event), end(in_next_event),
                   std::back_inserter(missing_up));

    std::vector<MirTouchId> missing_down;
    set_difference(begin(changed_in_next_event), end(changed_in_next_event),
                   begin(expected), end(expected),
                   std::back_inserter(missing_down));

    for (auto touch_id : missing_up)
    {
        // TODO on purpose we only send out one state change per event..
        auto pos = find_if(begin(last_state), end(last_state), [touch_id](auto& item){return item.touch_id == touch_id;});
        pos->action = mir_touch_action_up;
        dispatch(last_state);
        last_state.erase(pos);
    }

    for (auto touch_id : missing_down)
    {
        // TODO on purpose we only send out one state change per event..
        auto pos = find_if(begin(new_event), end(new_event), [touch_id](auto& item){return item.touch_id == touch_id;});
        last_state.push_back(*pos);
        last_state.back().action = mir_touch_action_down;
        dispatch(last_state);
        last_state.back().action = mir_touch_action_change;
    }
}

void mi::Validator::handle_touch_event(MirEvent const& event)
{
    std::lock_guard<std::mutex> lg(state_guard);

    auto const input_event = mir_event_get_input_event(&event);
    auto const id = mir_input_event_get_device_id(input_event);
    auto const touch_event = mir_input_event_get_touch_event(input_event);
    auto new_state = get_contact_state(touch_event);

    ensure_stream_validity_locked(
        lg,
        new_state,
        last_event_by_device[id],
        [this, touch_event, id](std::vector<events::ContactState> const& contacts)
        {
            dispatch_valid_event(*mev::make_event(
                    id,
                    std::chrono::nanoseconds{mir_touch_event_modifiers(touch_event)},
                    std::vector<uint8_t>{},
                    mir_touch_event_modifiers(touch_event),
                    contacts
                    ));
        });

    dispatch_valid_event(event);

    new_state.erase(
        remove_if(
            begin(new_state),
            end(new_state),
            [](auto& item)
            {
                if (item.action == mir_touch_action_down)
                   item.action = mir_touch_action_change;
                return item.action == mir_touch_action_up;
            }),
        end(new_state)
        );

    last_event_by_device[id] = new_state;
}
