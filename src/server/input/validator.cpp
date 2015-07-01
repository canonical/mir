/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/input/validator.h"

#include "mir_toolkit/event.h"

#include "mir/events/event_private.h"

#include <string.h>

#include <unordered_set>

namespace mi = mir::input;
namespace mev = mir::events;

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
    auto tev = mir_input_event_get_touch_event(iev);
    handle_touch_event(mir_input_event_get_device_id(iev), tev);

    return;
}

namespace
{
void delete_event(MirEvent *e) { mir_event_unref(e); }
mir::EventUPtr make_event_uptr(MirEvent *e)
{
    return mir::EventUPtr(e, delete_event);
}

mir::EventUPtr copy_event(MirTouchEvent const* ev)
{
    MirEvent *ret = new MirEvent;
    memcpy(ret, ev, sizeof(MirEvent));
    return make_event_uptr(ret);
}

// Return a copy of ev with existing touch actions converted to change.
// Note this is always a valid successor of ev in the event stream.
mir::EventUPtr convert_touch_actions_to_change(MirTouchEvent const* ev)
{
    auto ret = copy_event(ev);

    for (size_t i = 0; i < ret->motion.pointer_count; i++)
    {
        ret->motion.pointer_coordinates[i].action = mir_touch_action_change;
    }
    return ret;
}

// Helper function to find index for a given touch ID
// TODO: Existence of this probably suggests a problem with TouchEvent API...
int index_for_id(MirTouchEvent const* touch_ev, MirTouchId id)
{
    MirEvent const* ev = reinterpret_cast<MirEvent const*>(touch_ev);
    for (size_t i = 0; i < ev->motion.pointer_count; i++)
    {
        if (ev->motion.pointer_coordinates[i].id == id)
            return i;
    }
    return -1;
}

// Return an event which is a valid successor of valid_ev but contains a mir_touch_action_down for missing_id
mir::EventUPtr add_missing_down(MirEvent const* valid_ev, MirTouchEvent const* ev, MirTouchId missing_id)
{
    auto valid_tev = reinterpret_cast<MirTouchEvent const*>(valid_ev);
    
    // So as not to repeat already occurred actions, we copy the last valid (Delivered) event and replace all the actions
    // with change, then we will add a missing down for touch "missing_id"
    auto ret = convert_touch_actions_to_change(valid_tev);
    auto index = index_for_id(ev, missing_id);

    // In the case where a touch ID goes up and then reappears without a down we don't want to
    // add a new touch but merely replace the action in the last event.
    if (auto existing_index = index_for_id((MirTouchEvent const*)ret.get(), missing_id) >= 0)
    {
        ret->motion.pointer_coordinates[existing_index].action = mir_touch_action_down;
    }
    else
    {
        mev::add_touch(*ret, missing_id, mir_touch_action_down,
            mir_touch_event_tooltype(ev, index),
            mir_touch_event_axis_value(ev, index, mir_touch_axis_x),
            mir_touch_event_axis_value(ev, index, mir_touch_axis_y),
            mir_touch_event_axis_value(ev, index, mir_touch_axis_pressure),
            mir_touch_event_axis_value(ev, index, mir_touch_axis_touch_major),
            mir_touch_event_axis_value(ev, index, mir_touch_axis_touch_minor),
            mir_touch_event_axis_value(ev, index, mir_touch_axis_size));
    }
    
  return ret;
}

// We copy valid_ev but replace the touch action for missign_an_up_id with mir_touch_action_up
mir::EventUPtr add_missing_up(MirEvent const* valid_ev, MirTouchId missing_an_up_id)
{
    auto tev = reinterpret_cast<MirTouchEvent const*>(valid_ev);
    
    // Because we can only have one action per ev, we copy the last valid (Delivered) event and replace all the actions
    // with change, then we will change the action for the ID which should have gone up to an up.
    auto ret = convert_touch_actions_to_change(tev);
    auto index = index_for_id(tev, missing_an_up_id);

    ret->motion.pointer_coordinates[index].action = mir_touch_action_up;

    return ret;
}

// We copy ev but remove touch points which have been released producing a valid successor of
// ev
mir::EventUPtr remove_old_releases_from(MirEvent const* ev)
{
    auto tev = reinterpret_cast<MirTouchEvent const*>(ev);
    auto ret = copy_event(tev);
    ret->motion.pointer_count = 0;
    
    for (size_t i = 0; i < mir_touch_event_point_count(tev); i++)
    {
        auto action = mir_touch_event_action(tev, i);
        if (action == mir_touch_action_up)
            continue;
        mev::add_touch(*ret,
            mir_touch_event_id(tev, i), mir_touch_event_action(tev, i),
            mir_touch_event_tooltype(tev, i),
            mir_touch_event_axis_value(tev, i, mir_touch_axis_x),
            mir_touch_event_axis_value(tev, i, mir_touch_axis_y),
            mir_touch_event_axis_value(tev, i, mir_touch_axis_pressure),
            mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_major),
            mir_touch_event_axis_value(tev, i, mir_touch_axis_touch_minor),
            mir_touch_event_axis_value(tev, i, mir_touch_axis_size));
    }
    return ret;
}
}

typedef std::unordered_set<MirTouchId> TouchSet;

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
void mi::Validator::ensure_stream_validity_locked(std::lock_guard<std::mutex> const&,
    MirTouchEvent const* ev, MirTouchEvent const* last_ev)
{
    TouchSet expected;
    for (size_t i = 0; i < mir_touch_event_point_count(last_ev); i++)
    {
        auto action = mir_touch_event_action(last_ev, i);
        if (action == mir_touch_action_up)
            continue;
        expected.insert(mir_touch_event_id(last_ev, i));
    }

    TouchSet found;
    for (size_t i = 0; i < mir_touch_event_point_count(ev); i++)
    {
        auto id = mir_touch_event_id(ev, i);
        found.insert(id);
    }

    // Insert missing touch releases
    TouchSet missing;
    auto last_ev_copy =
        remove_old_releases_from(reinterpret_cast<MirEvent const*>(last_ev));
    for (auto const& expected_id : expected)
    {
        if (found.find(expected_id) == found.end())
        {
            auto inject_ev = add_missing_up(last_ev_copy.get(), expected_id);
            dispatch_valid_event(*inject_ev);
            last_ev_copy = remove_old_releases_from(inject_ev.get());
        }
    }
    

    for (size_t i = 0; i < mir_touch_event_point_count(ev); i++)
    {
        auto id = mir_touch_event_id(ev, i);
        if (expected.find(id) == expected.end() &&
            mir_touch_event_action(ev, i) != mir_touch_action_down)
        {
            
            auto inject_ev = add_missing_down(last_ev_copy.get(), ev, id);
            dispatch_valid_event(*inject_ev);
            last_ev_copy = std::move(inject_ev);
        }
    }

}


void mi::Validator::handle_touch_event(MirInputDeviceId id, MirTouchEvent const* ev)
{
    std::lock_guard<std::mutex> lg(state_guard);

    auto it = last_event_by_device.find(id);
    MirTouchEvent const* last_ev = nullptr;
    auto default_ev = mev::make_event(id,
        std::chrono::high_resolution_clock::now().time_since_epoch(),
        mir_input_event_modifier_none); 

    if (it == last_event_by_device.end())
    {
        last_event_by_device.insert(std::make_pair(id, copy_event(ev)));
        last_ev = reinterpret_cast<MirTouchEvent const*>(default_ev.get());
    }
    else
    {
        last_ev = mir_input_event_get_touch_event(mir_event_get_input_event(it->second.get()));
    }
    
    ensure_stream_validity_locked(lg, ev, last_ev);

    // Seems to be no better way to replace a non default constructible value type in an unordered_map
    // C++17 will give us insert_or_assign
    last_event_by_device.erase(id);
    last_event_by_device.insert(std::make_pair(id, copy_event(ev)));
    
    dispatch_valid_event(*reinterpret_cast<MirEvent const*>(ev));
}
