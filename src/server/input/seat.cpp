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

#include "seat.h"
#include "mir/input/device.h"
#include "mir/input/cursor_listener.h"
#include "mir/input/input_region.h"
#include "mir/geometry/displacement.h"

#include "input_modifier_utils.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <algorithm>

namespace mi = mir::input;

mi::Seat::Seat(std::shared_ptr<TouchVisualizer> const& touch_visualizer,
               std::shared_ptr<CursorListener> const& cursor_listener,
               std::shared_ptr<InputRegion> const& input_region)
    : touch_visualizer{touch_visualizer}, cursor_listener{cursor_listener}, input_region{input_region}, modifier{0}
{
}

void mi::Seat::add_device(MirInputDeviceId id)
{
    device_data[id];
}

void mi::Seat::remove_device(MirInputDeviceId id)
{
    auto stored_data = device_data.find(id);

    if (stored_data == end(device_data))
        BOOST_THROW_EXCEPTION(std::logic_error("Modifier for unknown device changed"));

    bool mod_update_needed = stored_data->second.mod != mir_input_event_modifier_none;
    bool spot_update_needed = !stored_data->second.spots.empty();

    device_data.erase(stored_data);

    if (mod_update_needed)
        update_modifier();
    if (spot_update_needed)
        update_spots();
}

MirInputEventModifiers mi::Seat::event_modifier() const
{
    return expand_modifiers(modifier);
}

MirInputEventModifiers mi::Seat::event_modifier(MirInputDeviceId id) const
{
    auto stored_data = device_data.find(id);
    if (stored_data == end(device_data))
        BOOST_THROW_EXCEPTION(std::logic_error("Modifier for unknown device requested"));
    return expand_modifiers(stored_data->second.mod);
}

void mi::Seat::update_seat_properties(MirInputEvent const* event)
{
    auto id = mir_input_event_get_device_id(event);

    auto stored_data = device_data.find(id);

    if (stored_data == end(device_data))
        BOOST_THROW_EXCEPTION(std::logic_error("Modifier for unknown device changed"));

    switch(mir_input_event_get_type(event))
    {
    case mir_input_event_type_key:
        {
            auto const* key = mir_input_event_get_keyboard_event(event);
            if (stored_data->second.update_modifier(mir_keyboard_event_action(key),
                                                    mir_keyboard_event_scan_code(key)))
                update_modifier();
            break;
        }
    case mir_input_event_type_touch:
        if (stored_data->second.update_spots(mir_input_event_get_touch_event(event)))
            update_spots();
        break;
    case mir_input_event_type_pointer:
        update_cursor(mir_input_event_get_pointer_event(event));
        break;
    default:
        break;
    }
}

bool mi::Seat::DeviceData::update_modifier(MirKeyboardAction key_action, int scan_code)
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

bool mi::Seat::DeviceData::update_spots(MirTouchEvent const* event)
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

void mi::Seat::update_spots()
{
    spots.clear();
    for (auto const& dev : device_data)
        spots.insert(end(spots), begin(dev.second.spots), end(dev.second.spots));

    touch_visualizer->visualize_touches(spots);
}

void mi::Seat::update_modifier()
{
    modifier = std::accumulate(begin(device_data),
                               end(device_data),
                               MirInputEventModifiers{0},
                               [](auto const& acc, auto const& item)
                               {
                                   return acc | item.second.mod;
                               });
}

mir::geometry::Point mi::Seat::cursor_position() const
{
    return cursor_pos;
}

void mi::Seat::update_cursor(MirPointerEvent const* event)
{
    mir::geometry::Displacement movement{
        mir_pointer_event_axis_value(event, mir_pointer_axis_relative_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_relative_y),
    };
    auto new_position = cursor_pos + movement;
    input_region->confine(new_position);
    movement = new_position - cursor_pos;

    cursor_pos = new_position;

    cursor_listener->cursor_moved_to(cursor_pos.x.as_float(), cursor_pos.y.as_float());
}
