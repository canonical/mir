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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "input_device.h"

#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/input_device_info.h"
#include "mir/input/device_capability.h"
#include "mir/input/event_builder.h"
#include "mir/input/input_sink.h"

#include <X11/Xlib.h>
#include <iostream>

namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mix = mi::X;

namespace
{
MirPointerButtons to_button_state(int button)
{
    if (button == Button1)
        return mir_pointer_button_primary;
    if (button == Button2)  // tertiary (middle) button is Button2 in X
        return mir_pointer_button_tertiary;
    if (button == Button3)
        return mir_pointer_button_secondary;
    if (button == 8)
        return mir_pointer_button_side;
    if (button == 9)
        return mir_pointer_button_extra;
    return 0;
}

}

mix::XInputDevice::XInputDevice(InputDeviceInfo const& device_info)
    : info(device_info)
{
}

void mix::XInputDevice::start(InputSink* input_sink, EventBuilder* event_builder)
{
    sink = input_sink;
    builder = event_builder;
}

void mix::XInputDevice::stop()
{
    sink = nullptr;
    builder = nullptr;
}

mi::InputDeviceInfo mix::XInputDevice::get_device_info()
{
    return info;
}

mir::optional_value<mi::PointerSettings> mix::XInputDevice::get_pointer_settings() const
{
    mir::optional_value<PointerSettings> ret;
    if (contains(info.capabilities, DeviceCapability::pointer))
        ret = PointerSettings();

    return ret;
}

void mix::XInputDevice::apply_settings(PointerSettings const&)
{
    // TODO Make use if X11-XInput2
}

mir::optional_value<mi::TouchpadSettings> mix::XInputDevice::get_touchpad_settings() const
{
    optional_value<TouchpadSettings> ret;
    if (contains(info.capabilities, DeviceCapability::touchpad))
        ret = TouchpadSettings();

    return ret;
}

void mix::XInputDevice::apply_settings(TouchpadSettings const&)
{
}

bool mix::XInputDevice::started() const
{
    return sink && builder;
}

void mix::XInputDevice::key_press(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code)
{
    sink->handle_input(
        *builder->key_event(
            event_time,
            mir_keyboard_action_down,
            key_sym,
            key_code
            )
        );

}

void mix::XInputDevice::key_release(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code)
{
    sink->handle_input(
        *builder->key_event(
            event_time,
            mir_keyboard_action_up,
            key_sym,
            key_code
            )
        );
}

void mix::XInputDevice::pointer_press(std::chrono::nanoseconds event_time, int button, mir::geometry::Point const& pos, mir::geometry::Displacement scroll)
{
    button_state |= to_button_state(button);

    auto const movement = pos - pointer_pos;
    pointer_pos = pos;
    sink->handle_input(
        *builder->pointer_event(
            event_time,
            mir_pointer_action_button_down,
            button_state,
            scroll.dx.as_float(),
            scroll.dy.as_float(),
            movement.dx.as_float(),
            movement.dy.as_float()
            )
        );
}

void mix::XInputDevice::pointer_release(std::chrono::nanoseconds event_time, int button, mir::geometry::Point const& pos, mir::geometry::Displacement scroll)
{
    button_state = button_state & ~(to_button_state(button));

    auto const movement = pos - pointer_pos;
    pointer_pos = pos;
    sink->handle_input(
        *builder->pointer_event(
            event_time,
            mir_pointer_action_button_up,
            button_state,
            scroll.dx.as_float(),
            scroll.dy.as_float(),
            movement.dx.as_float(),
            movement.dy.as_float()
            )
        );

}

void mix::XInputDevice::pointer_motion(std::chrono::nanoseconds event_time, mir::geometry::Point const& pos, mir::geometry::Displacement scroll)
{
    std::cout << "pos " << pos.x.as_float() << std::endl;
    auto const movement = pos - pointer_pos;
    pointer_pos = pos;
    sink->handle_input(
        *builder->pointer_event(
            event_time,
            mir_pointer_action_motion,
            button_state,
            scroll.dx.as_float(),
            scroll.dy.as_float(),
            movement.dx.as_float(),
            movement.dy.as_float()
            )
        );
}
