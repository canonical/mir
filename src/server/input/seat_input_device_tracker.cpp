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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "seat_input_device_tracker.h"
#include "mir/input/device.h"
#include "mir/input/cursor_listener.h"
#include "mir/input/input_dispatcher.h"
#include "mir/input/key_mapper.h"
#include "mir/input/seat_observer.h"
#include "mir/geometry/displacement.h"
#include "mir/events/event_builders.h"
#include "mir/time/clock.h"

#include "input_modifier_utils.h"

#include <boost/throw_exception.hpp>
#include <linux/input.h>

#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <numeric>

namespace mi = mir::input;
namespace mev = mir::events;
namespace geom = mir::geometry;

mi::SeatInputDeviceTracker::SeatInputDeviceTracker(std::shared_ptr<InputDispatcher> const& dispatcher,
                                                   std::shared_ptr<TouchVisualizer> const& touch_visualizer,
                                                   std::shared_ptr<CursorListener> const& cursor_listener,
                                                   std::shared_ptr<KeyMapper> const& key_mapper,
                                                   std::shared_ptr<time::Clock> const& clock,
                                                   std::shared_ptr<SeatObserver> const& observer)
    : dispatcher{dispatcher}, touch_visualizer{touch_visualizer}, cursor_listener{cursor_listener},
      key_mapper{key_mapper}, clock{clock}, observer{observer}, buttons{0}
{
}

void mi::SeatInputDeviceTracker::add_device(MirInputDeviceId id)
{
    {
        std::lock_guard<std::mutex> lock(device_state_mutex);
        device_data[id];
    }
    observer->seat_add_device(id);
}

void mi::SeatInputDeviceTracker::remove_device(MirInputDeviceId id)
{
    {
        std::lock_guard<std::mutex> lock(device_state_mutex);
        auto stored_data = device_data.find(id);

        if (stored_data == end(device_data))
            BOOST_THROW_EXCEPTION(std::logic_error("Modifier for unknown device changed"));

        bool state_update_needed = stored_data->second.buttons != 0;
        bool spot_update_needed = !stored_data->second.spots.empty();

        device_data.erase(stored_data);
        key_mapper->clear_keymap_for_device(id);

        if (state_update_needed)
            update_states();
        if (spot_update_needed)
            update_spots();
    }

    observer->seat_remove_device(id);
}

void mi::SeatInputDeviceTracker::dispatch(std::shared_ptr<MirEvent> const& event)
{
    if (mir_event_get_type(event.get()) == mir_event_type_input)
    {
        std::lock_guard<std::mutex> lock(device_state_mutex);

        auto input_event = mir_event_get_input_event(event.get());

        if (filter_input_event(input_event))
            return;

        update_seat_properties(input_event);

        key_mapper->map_event(*event);

        if (mir_input_event_type_pointer == mir_input_event_get_type(input_event))
        {
            mev::set_cursor_position(*event, cursor_x, cursor_y);
            mev::set_button_state(*event, buttons);
        }
    }

    dispatcher->dispatch(event);
    observer->seat_dispatch_event(event);
}

bool mi::SeatInputDeviceTracker::filter_input_event(MirInputEvent const* event)
{
    auto device_id = mir_input_event_get_device_id(event);
    auto type = mir_input_event_get_type(event);
    if (type == mir_input_event_type_key)
    {
        auto stored_data = device_data.find(device_id);

        if (stored_data == end(device_data))
            return true;

        return !stored_data->second.allowed_scan_code_action(mir_input_event_get_keyboard_event(event));;
    }
    return false;
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
        stored_data->second.update_scan_codes(mir_input_event_get_keyboard_event(event));
        break;
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
    buttons = std::accumulate(begin(device_data),
                              end(device_data),
                              MirPointerButtons{0},
                              [](auto const& acc, auto const& item) { return acc | item.second.buttons; });
}

mir::geometry::Point mi::SeatInputDeviceTracker::cursor_position() const
{
    std::lock_guard<std::mutex> lock(device_state_mutex);
    return {cursor_x, cursor_y};
}

MirPointerButtons mi::SeatInputDeviceTracker::button_state() const
{
    std::lock_guard<std::mutex> lock(device_state_mutex);
    return buttons;
}

void mi::SeatInputDeviceTracker::set_confinement_regions(geometry::Rectangles const& regions)
{
    {
        std::lock_guard<std::mutex> lg(region_mutex);
        confined_region = regions;
    }
    observer->seat_set_confinement_region_called(regions);
}

void mi::SeatInputDeviceTracker::reset_confinement_regions()
{
    {
        std::lock_guard<std::mutex> lg(region_mutex);
        confined_region.clear();
    }
    observer->seat_reset_confinement_regions();
}

void mi::SeatInputDeviceTracker::update_outputs(geom::Rectangles const& output_regions)
{
    std::lock_guard<std::mutex> lg(output_mutex);
    input_region = output_regions;
}

void mi::SeatInputDeviceTracker::confine_function(mir::geometry::Point& p) const
{
    std::lock_guard<std::mutex> lg(region_mutex);
    input_region.confine(p);
    confined_region.confine(p);
}

void mi::SeatInputDeviceTracker::confine_pointer()
{
    mir::geometry::Point const old{cursor_x, cursor_y};
    auto confined = old;
    confine_function(confined);
    if (confined.x != old.x) cursor_x = confined.x.as_int();
    if (confined.y != old.y) cursor_y = confined.y.as_int();
}

void mi::SeatInputDeviceTracker::update_cursor(MirPointerEvent const* event)
{
    cursor_x += mir_pointer_event_axis_value(event, mir_pointer_axis_relative_x);
    cursor_y += mir_pointer_event_axis_value(event, mir_pointer_axis_relative_y);

    confine_pointer();

    cursor_listener->cursor_moved_to(cursor_x, cursor_y);
}

mir::EventUPtr mi::SeatInputDeviceTracker::create_device_state() const
{
    std::lock_guard<std::mutex> lock(device_state_mutex);
    std::vector<mev::InputDeviceState> devices;
    devices.reserve(device_data.size());
    for (auto const& item : device_data)
    {
        devices.push_back({item.first, item.second.scan_codes, item.second.buttons});
        auto lock_state = key_mapper->device_modifiers(item.first);

        bool caps_lock_active = (lock_state & mir_input_event_modifier_caps_lock);
        bool scroll_lock_active = (lock_state & mir_input_event_modifier_scroll_lock);
        bool num_lock_active = (lock_state & mir_input_event_modifier_num_lock);

        bool contains_caps_lock_pressed = false;
        bool contains_scroll_lock_pressed = false;
        bool contains_num_lock_pressed = false;

        for (uint32_t scan_code : item.second.scan_codes)
        {
            if (scan_code == KEY_CAPSLOCK)
                contains_caps_lock_pressed = true;
            else if (scan_code == KEY_SCROLLLOCK)
                contains_scroll_lock_pressed = true;
            else if (scan_code == KEY_NUMLOCK)
                contains_num_lock_pressed = true;
        }

        if (caps_lock_active && !contains_caps_lock_pressed)
        {
            devices.back().pressed_keys.push_back(KEY_CAPSLOCK);
            devices.back().pressed_keys.push_back(KEY_CAPSLOCK);
        }
        if (num_lock_active && !contains_num_lock_pressed)
        {
            devices.back().pressed_keys.push_back(KEY_NUMLOCK);
            devices.back().pressed_keys.push_back(KEY_NUMLOCK);
        }
        if (scroll_lock_active && !contains_scroll_lock_pressed)
        {
            devices.back().pressed_keys.push_back(KEY_SCROLLLOCK);
            devices.back().pressed_keys.push_back(KEY_SCROLLLOCK);
        }
    }
    auto out_ev = mev::make_event(
        clock->now().time_since_epoch(),
        buttons,
        key_mapper->modifiers(),
        cursor_x,
        cursor_y,
        std::move(devices));

    return out_ev;
}

void mi::SeatInputDeviceTracker::DeviceData::update_scan_codes(MirKeyboardEvent const* event)
{
    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    if (action == mir_keyboard_action_down)
        scan_codes.push_back(scan_code);
    else if (action == mir_keyboard_action_up)
        scan_codes.erase(remove(begin(scan_codes), end(scan_codes), scan_code), end(scan_codes));
}

void mi::SeatInputDeviceTracker::set_key_state(MirInputDeviceId id, std::vector<uint32_t> const& scan_codes)
{
    {
        std::lock_guard<std::mutex> lock(device_state_mutex);
        key_mapper->set_key_state(id, scan_codes);

        auto device = device_data.find(id);

        if (device != end(device_data))
            device->second.scan_codes = scan_codes;
    }

    observer->seat_set_key_state(id, scan_codes);
}

void mi::SeatInputDeviceTracker::set_pointer_state(MirInputDeviceId id, MirPointerButtons buttons)
{
    {
        std::lock_guard<std::mutex> lock(device_state_mutex);
        auto device = device_data.find(id);

        if (device != end(device_data))
            device->second.update_button_state(buttons);
    }

    observer->seat_set_pointer_state(id, buttons);
}

void mi::SeatInputDeviceTracker::set_cursor_position(float x, float y)
{
    {
        std::lock_guard<std::mutex> lock(device_state_mutex);
        cursor_x = x;
        cursor_y = y;
    }

    observer->seat_set_cursor_position(x, y);
}

bool mi::SeatInputDeviceTracker::DeviceData::allowed_scan_code_action(MirKeyboardEvent const* event) const
{
    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    bool found = find(begin(scan_codes), end(scan_codes), scan_code) != end(scan_codes);

    return (action == mir_keyboard_action_down && !found)
        || (action != mir_keyboard_action_down && found);
}
