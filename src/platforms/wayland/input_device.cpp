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

#include <linux/input.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mi = mir::input;
namespace miw = mi::wayland;

namespace
{
// {arg} TODO: this is ripped off from src/platforms/evdev/button_utils.cpp it ought to be shared
MirPointerButton to_pointer_button(int button, MirPointerHandedness handedness)
{
    switch(button)
    {
    case BTN_LEFT: return (handedness == mir_pointer_handedness_right)
                          ? mir_pointer_button_primary
                          : mir_pointer_button_secondary;
    case BTN_RIGHT: return (handedness == mir_pointer_handedness_right)
                           ? mir_pointer_button_secondary
                           : mir_pointer_button_primary;
    case BTN_MIDDLE: return mir_pointer_button_tertiary;
    case BTN_BACK: return mir_pointer_button_back;
    case BTN_FORWARD: return mir_pointer_button_forward;
    case BTN_SIDE: return mir_pointer_button_side;
    case BTN_EXTRA: return mir_pointer_button_extra;
    case BTN_TASK: return mir_pointer_button_task;
    }
    BOOST_THROW_EXCEPTION(std::runtime_error("Invalid mouse button"));
}
}

miw::InputDevice::InputDevice(
    InputDeviceInfo const& device_info,
    std::shared_ptr<dispatch::ActionQueue> const& action_queue) :
    info(device_info),
    action_queue{action_queue}
{
}

void miw::InputDevice::start(InputSink* input_sink, EventBuilder* event_builder)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    sink = input_sink;
    builder = event_builder;
}

void miw::InputDevice::stop()
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    sink = nullptr;
    builder = nullptr;
}

mi::InputDeviceInfo miw::InputDevice::get_device_info()
{
    return info;
}

mir::optional_value<mi::PointerSettings> miw::InputDevice::get_pointer_settings() const
{
    mir::optional_value<PointerSettings> ret;
    if (contains(info.capabilities, DeviceCapability::pointer))
        ret = PointerSettings();

    return ret;
}

void miw::InputDevice::apply_settings(PointerSettings const&)
{
}

mir::optional_value<mi::TouchpadSettings> miw::InputDevice::get_touchpad_settings() const
{
    optional_value<TouchpadSettings> ret;
    if (contains(info.capabilities, DeviceCapability::touchpad))
        ret = TouchpadSettings();

    return ret;
}

void miw::InputDevice::apply_settings(TouchpadSettings const&)
{
}

mir::optional_value<mi::TouchscreenSettings> miw::InputDevice::get_touchscreen_settings() const
{
    optional_value<TouchscreenSettings> ret;
    if (contains(info.capabilities, DeviceCapability::touchscreen))
        ret = TouchscreenSettings();

    return ret;
}

void miw::InputDevice::apply_settings(TouchscreenSettings const&)
{
}

bool miw::InputDevice::started() const
{
    return sink && builder;
}

void  miw::InputDevice::enqueue(std::function<EventUPtr(EventBuilder* builder)> const& event)
{
    action_queue->enqueue([=]
          {
              std::lock_guard<decltype(mutex)> lock{mutex};
              if (started())
                  sink->handle_input(event(builder));
          });
}

void miw::InputDevice::key_press(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code)
{
    enqueue([=](EventBuilder* b) { return b->key_event(event_time, mir_keyboard_action_down, key_sym, key_code); });
}

void miw::InputDevice::key_release(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code)
{
    enqueue([=](EventBuilder* b) { return b->key_event(event_time, mir_keyboard_action_up, key_sym, key_code); });
}

void miw::InputDevice::pointer_press(std::chrono::nanoseconds event_time, int button, geometry::Point const& pos, geometry::Displacement scroll)
{
    enqueue([=](EventBuilder* b)
    {
        button_state |= to_pointer_button(button, mir_pointer_handedness_right);

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

void miw::InputDevice::pointer_release(std::chrono::nanoseconds event_time, int button, geometry::Point const& pos, geometry::Displacement scroll)
{
    enqueue([=](EventBuilder* b)
    {
        button_state &= ~to_pointer_button(button, mir_pointer_handedness_right);

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

void miw::InputDevice::pointer_motion(std::chrono::nanoseconds event_time, geometry::Point const& pos, geometry::Displacement scroll)
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

void mir::input::wayland::InputDevice::touch_event(
    std::chrono::nanoseconds event_time, std::vector<events::ContactState> const& contacts)
{
    enqueue([=](EventBuilder* b)
    {
        return b->touch_event(event_time, contacts);
    });
}
