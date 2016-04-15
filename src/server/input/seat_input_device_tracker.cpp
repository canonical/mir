/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "seat_input_device_tracker.h"
#include "mir/input/device.h"
#include "mir/input/cursor_listener.h"
#include "mir/input/input_region.h"
#include "mir/input/input_dispatcher.h"
#include "mir/geometry/displacement.h"
#include "mir/events/event_builders.h"

#include "input_modifier_utils.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <numeric>

namespace mi = mir::input;
namespace mev = mir::events;

mi::SeatInputDeviceTracker::SeatInputDeviceTracker(std::shared_ptr<InputDispatcher> const& dispatcher,
                                                   std::shared_ptr<TouchVisualizer> const& touch_visualizer,
                                                   std::shared_ptr<CursorListener> const& cursor_listener,
                                                   std::shared_ptr<InputRegion> const& input_region)
    : dispatcher{dispatcher}, touch_visualizer{touch_visualizer}, cursor_listener{cursor_listener},
      input_region{input_region}, modifier{0}, buttons{0}
{
}

void mi::SeatInputDeviceTracker::add_device(MirInputDeviceId id)
{
    device_data[id];
}

void mi::SeatInputDeviceTracker::remove_device(MirInputDeviceId id)
{
    auto stored_data = device_data.find(id);

    if (stored_data == end(device_data))
        BOOST_THROW_EXCEPTION(std::logic_error("Modifier for unknown device changed"));

    bool state_update_needed = stored_data->second.mod != mir_input_event_modifier_none ||
        stored_data->second.buttons != 0;
    bool spot_update_needed = !stored_data->second.spots.empty();

    device_data.erase(stored_data);

    if (state_update_needed)
        update_states();
    if (spot_update_needed)
        update_spots();
}

void mi::SeatInputDeviceTracker::dispatch(MirEvent &event)
{
    auto input_event = mir_event_get_input_event(&event);
    update_seat_properties(input_event);

    if (mir_input_event_type_key  == mir_input_event_get_type(input_event))
        mev::set_modifier(event, event_modifier(mir_input_event_get_device_id(input_event)));
    else
        mev::set_modifier(event, event_modifier());

    if (mir_input_event_type_pointer == mir_input_event_get_type(input_event))
    {
        mev::set_cursor_position(event, cursor_position());
        mev::set_button_state(event, button_state());
    }

    dispatcher->dispatch(event);
}

MirInputEventModifiers mi::SeatInputDeviceTracker::event_modifier() const
{
    return expand_modifiers(modifier);
}

MirInputEventModifiers mi::SeatInputDeviceTracker::event_modifier(MirInputDeviceId id) const
{
    auto stored_data = device_data.find(id);
    if (stored_data == end(device_data))
        BOOST_THROW_EXCEPTION(std::logic_error("Modifier for unknown device requested"));
    return expand_modifiers(stored_data->second.mod);
}

void mi::SeatInputDeviceTracker::update_seat_properties(MirInputEvent const* event)
{
    auto id = mir_input_event_get_device_id(event);

    auto stored_data = device_data.find(id);

    if (stored_data == end(device_data))
        BOOST_THROW_EXCEPTION(std::logic_error("Event of unknown device received"));

    switch(mir_input_event_get_type(event))
    {
    case mir_input_event_type_key:
        {
            auto const* key = mir_input_event_get_keyboard_event(event);
            if (stored_data->second.update_modifier(mir_keyboard_event_action(key),
                                                    mir_keyboard_event_scan_code(key)))
                update_states();
            break;
        }
    case mir_input_event_type_touch:
        if (stored_data->second.update_spots(mir_input_event_get_touch_event(event)))
            update_spots();
        break;
    case mir_input_event_type_pointer:
        {
            auto const* pointer = mir_input_event_get_pointer_event(event);
            update_cursor(pointer);
            if(stored_data->second.update_button_state(mir_pointer_event_buttons(pointer)))
                update_states();
            break;
        }
    default:
        break;
    }
}

bool mi::SeatInputDeviceTracker::DeviceData::update_modifier(MirKeyboardAction key_action, int scan_code)
{
    auto mod_change = to_modifiers(scan_code);

    if (mod_change == 0 || key_action == mir_keyboard_action_repeat)
        return false;

    if (key_action == mir_keyboard_action_down)
        mod |= mod_change;
    else if (key_action == mir_keyboard_action_up)
        mod &= ~mod_change;

    return true;
}

bool mi::SeatInputDeviceTracker::DeviceData::update_button_state(MirPointerButtons button_state)
{
    if (buttons == button_state)
        return false;
    buttons = button_state;

    return true;
}

bool mi::SeatInputDeviceTracker::DeviceData::update_spots(MirTouchEvent const* event)
{
    auto count = mir_touch_event_point_count(event);
    spots.clear();
    for (decltype(count) i = 0; i != count; ++i)
    {
        if (mir_touch_event_action(event, i) == mir_touch_action_up)
            continue;
        spots.push_back({{mir_touch_event_axis_value(event, i, mir_touch_axis_x),
                          mir_touch_event_axis_value(event, i, mir_touch_axis_y)},
                         mir_touch_event_axis_value(event, i, mir_touch_axis_pressure)});
    }
    return true;
}

void mi::SeatInputDeviceTracker::update_spots()
{
    spots.clear();
    for (auto const& dev : device_data)
        spots.insert(end(spots), begin(dev.second.spots), end(dev.second.spots));

    touch_visualizer->visualize_touches(spots);
}

void mi::SeatInputDeviceTracker::update_states()
{
    std::tie(modifier, buttons) =
        std::accumulate(begin(device_data),
                        end(device_data),
                        std::make_pair(MirInputEventModifiers{0}, MirPointerButtons{0}),
                        [](auto const& acc, auto const& item)
                        {
                            return std::make_pair(acc.first | item.second.mod, acc.second | item.second.buttons);
                        });
}

mir::geometry::Point mi::SeatInputDeviceTracker::cursor_position() const
{
    return {cursor_x, cursor_y};
}

MirPointerButtons mi::SeatInputDeviceTracker::button_state() const
{
    return buttons;
}

void mi::SeatInputDeviceTracker::update_cursor(MirPointerEvent const* event)
{
    cursor_x +=
        mir_pointer_event_axis_value(event, mir_pointer_axis_relative_x);
    cursor_y +=
        mir_pointer_event_axis_value(event, mir_pointer_axis_relative_y);

    mir::geometry::Point confined{cursor_x, cursor_y};
    input_region->confine(confined);
    if (confined.x.as_int() != trunc(cursor_x))
        cursor_x = confined.x.as_int();
    if (confined.y.as_int() != trunc(cursor_y))
        cursor_y = confined.y.as_int();

    cursor_listener->cursor_moved_to(cursor_x, cursor_y);
}
