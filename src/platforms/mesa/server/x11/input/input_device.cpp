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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "input_device.h"

#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/touchscreen_settings.h"
#include "mir/input/input_device_info.h"
#include "mir/input/device_capability.h"
#include "mir/input/event_builder.h"
#include "mir/input/input_sink.h"

#include <X11/Xlib.h>

namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mix = mi::X;

namespace
{
MirPointerButtons to_mir_button(int button)
{
    auto const button_side = 8;
    auto const button_extra = 9;
    if (button == Button1)
        return mir_pointer_button_primary;
    if (button == Button2)  // tertiary (middle) button is Button2 in X
        return mir_pointer_button_tertiary;
    if (button == Button3)
        return mir_pointer_button_secondary;
    if (button == button_side)
        return mir_pointer_button_side;
    if (button == button_extra)
        return mir_pointer_button_extra;
    return 0;
}

MirPointerButtons to_mir_button_state(int x_button_key_state)
{
    MirPointerButtons button_state = 0;
    if (x_button_key_state & Button1Mask)
        button_state |= mir_pointer_button_primary;
    if (x_button_key_state & Button2Mask)
        button_state |= mir_pointer_button_tertiary;
    if (x_button_key_state & Button3Mask)
        button_state |= mir_pointer_button_secondary;
    if (x_button_key_state & Button4Mask)
        button_state |= mir_pointer_button_back;
    if (x_button_key_state & Button5Mask)
        button_state |= mir_pointer_button_forward;
    return button_state;
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
    // TODO Make use if X11-XInput2 to get raw device information
    // and support other devices than just the unified pointer and
    // keyboards.
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

mir::optional_value<mi::TouchscreenSettings> mix::XInputDevice::get_touchscreen_settings() const
{
    optional_value<TouchscreenSettings> ret;
    if (contains(info.capabilities, DeviceCapability::touchscreen))
        ret = TouchscreenSettings();

    return ret;
}

void mix::XInputDevice::apply_settings(TouchscreenSettings const&)
{
}

bool mix::XInputDevice::started() const
{
    return sink && builder;
}

void mix::XInputDevice::key_press(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code)
{
    sink->handle_input(
        builder->key_event(
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
        builder->key_event(
            event_time,
            mir_keyboard_action_up,
            key_sym,
            key_code
            )
        );
}

void mix::XInputDevice::update_button_state(int button)
{
    button_state = to_mir_button_state(button);
}

void mix::XInputDevice::pointer_press(std::chrono::nanoseconds event_time, int button, mir::geometry::Point const& pos, mir::geometry::Displacement scroll)
{
    button_state |= to_mir_button(button);

    auto const movement = pos - pointer_pos;
    pointer_pos = pos;
    sink->handle_input(
        builder->pointer_event(
            event_time,
            mir_pointer_action_button_down,
            button_state,
            pointer_pos.x.as_int(),
            pointer_pos.y.as_int(),
            scroll.dx.as_int(),
            scroll.dy.as_int(),
            movement.dx.as_int(),
            movement.dy.as_int()
            )
        );
}

void mix::XInputDevice::pointer_release(std::chrono::nanoseconds event_time, int button, mir::geometry::Point const& pos, mir::geometry::Displacement scroll)
{
    button_state &= ~to_mir_button(button);

    auto const movement = pos - pointer_pos;
    pointer_pos = pos;
    sink->handle_input(
        builder->pointer_event(
            event_time,
            mir_pointer_action_button_up,
            button_state,
            pointer_pos.x.as_int(),
            pointer_pos.y.as_int(),
            scroll.dx.as_int(),
            scroll.dy.as_int(),
            movement.dx.as_int(),
            movement.dy.as_int()
            )
        );

}

void mix::XInputDevice::pointer_motion(std::chrono::nanoseconds event_time, mir::geometry::Point const& pos, mir::geometry::Displacement scroll)
{
    auto const movement = pos - pointer_pos;
    pointer_pos = pos;
    sink->handle_input(
        builder->pointer_event(
            event_time,
            mir_pointer_action_motion,
            button_state,
            pointer_pos.x.as_int(),
            pointer_pos.y.as_int(),
            scroll.dx.as_int(),
            scroll.dy.as_int(),
            movement.dx.as_int(),
            movement.dy.as_int()
            )
        );
}
