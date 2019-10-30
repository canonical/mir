/*
 * Copyright © 2019 Canonical Ltd.
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
 */

#include "input_device.h"

#include <mir/dispatch/action_queue.h>
#include <mir/input/pointer_settings.h>
#include <mir/input/touchpad_settings.h>
#include <mir/input/touchscreen_settings.h>
#include <mir/input/input_device_info.h>
#include <mir/input/device_capability.h>
#include <mir/input/event_builder.h>
#include <mir/input/input_sink.h>

#include <X11/Xlib.h>

namespace mi = mir::input;
namespace mix = mi::wayland;

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

mix::InputDevice::InputDevice(
    InputDeviceInfo const& device_info,
    std::shared_ptr<dispatch::ActionQueue> const& action_queue) :
    info(device_info),
    action_queue{action_queue}
{
}

void mix::InputDevice::start(InputSink* input_sink, EventBuilder* event_builder)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    sink = input_sink;
    builder = event_builder;
}

void mix::InputDevice::stop()
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    sink = nullptr;
    builder = nullptr;
}

mi::InputDeviceInfo mix::InputDevice::get_device_info()
{
    return info;
}

mir::optional_value<mi::PointerSettings> mix::InputDevice::get_pointer_settings() const
{
    mir::optional_value<PointerSettings> ret;
    if (contains(info.capabilities, DeviceCapability::pointer))
        ret = PointerSettings();

    return ret;
}

void mix::InputDevice::apply_settings(PointerSettings const&)
{
}

mir::optional_value<mi::TouchpadSettings> mix::InputDevice::get_touchpad_settings() const
{
    optional_value<TouchpadSettings> ret;
    if (contains(info.capabilities, DeviceCapability::touchpad))
        ret = TouchpadSettings();

    return ret;
}

void mix::InputDevice::apply_settings(TouchpadSettings const&)
{
}

mir::optional_value<mi::TouchscreenSettings> mix::InputDevice::get_touchscreen_settings() const
{
    optional_value<TouchscreenSettings> ret;
    if (contains(info.capabilities, DeviceCapability::touchscreen))
        ret = TouchscreenSettings();

    return ret;
}

void mix::InputDevice::apply_settings(TouchscreenSettings const&)
{
}

bool mix::InputDevice::started() const
{
    return sink && builder;
}

void  mix::InputDevice::enqueue(std::function<EventUPtr(EventBuilder* builder)> const& event)
{
    action_queue->enqueue([=]
          {
              std::lock_guard<decltype(mutex)> lock{mutex};
              if (started())
                  sink->handle_input(event(builder));
          });
}

void mix::InputDevice::key_press(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code)
{
    enqueue([=](EventBuilder* b) { return b->key_event(event_time, mir_keyboard_action_down, key_sym, key_code); });
}

void mix::InputDevice::key_release(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code)
{
    enqueue([=](EventBuilder* b) { return b->key_event(event_time, mir_keyboard_action_up, key_sym, key_code); });
}

void mix::InputDevice::update_button_state(int button)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    button_state = to_mir_button_state(button);
}

void mix::InputDevice::pointer_press(std::chrono::nanoseconds event_time, int button, geometry::Point const& pos, geometry::Displacement scroll)
{
    enqueue([=](EventBuilder* b)
    {
        button_state |= to_mir_button(button);

        auto const movement = pos - pointer_pos;
        pointer_pos = pos;
        return b->pointer_event(
            event_time,
            mir_pointer_action_button_down,
            button_state,
            pointer_pos.x.as_int(),
            pointer_pos.y.as_int(),
            scroll.dx.as_int(),
            scroll.dy.as_int(),
            movement.dx.as_int(),
            movement.dy.as_int()
        );
    });
}

void mix::InputDevice::pointer_release(std::chrono::nanoseconds event_time, int button, geometry::Point const& pos, geometry::Displacement scroll)
{
    enqueue([=](EventBuilder* b)
    {
        button_state &= ~to_mir_button(button);

        auto const movement = pos - pointer_pos;
        pointer_pos = pos;
        return b->pointer_event(
            event_time,
            mir_pointer_action_button_up,
            button_state,
            pointer_pos.x.as_int(),
            pointer_pos.y.as_int(),
            scroll.dx.as_int(),
            scroll.dy.as_int(),
            movement.dx.as_int(),
            movement.dy.as_int()
        );
    });
}

void mix::InputDevice::pointer_motion(std::chrono::nanoseconds event_time, geometry::Point const& pos, geometry::Displacement scroll)
{
    enqueue([=](EventBuilder* b)
    {
        auto const movement = pos - pointer_pos;
        pointer_pos = pos;
        return b->pointer_event(
            event_time,
            mir_pointer_action_motion,
            button_state,
            pointer_pos.x.as_int(),
            pointer_pos.y.as_int(),
            scroll.dx.as_int(),
            scroll.dy.as_int(),
            movement.dx.as_int(),
            movement.dy.as_int()
        );
    });
}
