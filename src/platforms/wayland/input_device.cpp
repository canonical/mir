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
namespace geom = mir::geometry;

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

miw::GenericInputDevice::GenericInputDevice(
    InputDeviceInfo const& device_info,
    std::shared_ptr<dispatch::ActionQueue> const& action_queue) :
    info(device_info),
    action_queue{action_queue}
{
}

void miw::GenericInputDevice::start(InputSink* input_sink, EventBuilder* event_builder)
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    sink = input_sink;
    builder = event_builder;
}

void miw::GenericInputDevice::stop()
{
    std::lock_guard<decltype(mutex)> lock{mutex};
    sink = nullptr;
    builder = nullptr;
}

mi::InputDeviceInfo miw::GenericInputDevice::get_device_info()
{
    return info;
}

mir::optional_value<mi::PointerSettings> miw::GenericInputDevice::get_pointer_settings() const
{
    mir::optional_value<PointerSettings> ret;
    if (contains(info.capabilities, DeviceCapability::pointer))
        ret = PointerSettings();

    return ret;
}

void miw::GenericInputDevice::apply_settings(PointerSettings const&)
{
}

mir::optional_value<mi::TouchpadSettings> miw::GenericInputDevice::get_touchpad_settings() const
{
    optional_value<TouchpadSettings> ret;
    if (contains(info.capabilities, DeviceCapability::touchpad))
        ret = TouchpadSettings();

    return ret;
}

void miw::GenericInputDevice::apply_settings(TouchpadSettings const&)
{
}

mir::optional_value<mi::TouchscreenSettings> miw::GenericInputDevice::get_touchscreen_settings() const
{
    optional_value<TouchscreenSettings> ret;
    if (contains(info.capabilities, DeviceCapability::touchscreen))
        ret = TouchscreenSettings();

    return ret;
}

void miw::GenericInputDevice::apply_settings(TouchscreenSettings const&)
{
}

bool miw::GenericInputDevice::started() const
{
    return sink && builder;
}

void  miw::GenericInputDevice::enqueue(std::function<EventUPtr(EventBuilder* builder)> const& event)
{
    action_queue->enqueue([=, this]
          {
              std::lock_guard<decltype(mutex)> lock{mutex};
              if (started())
                  sink->handle_input(event(builder));
          });
}

miw::KeyboardInputDevice::KeyboardInputDevice(std::shared_ptr<dispatch::ActionQueue> const& action_queue) :
    GenericInputDevice{InputDeviceInfo{"keyboard-device", "key-dev-1", DeviceCapability::keyboard}, action_queue}
{
}

void miw::KeyboardInputDevice::key_press(std::chrono::nanoseconds event_time, xkb_keysym_t keysym, int32_t scan_code)
{
    enqueue([=](EventBuilder* b) { return b->key_event(event_time, mir_keyboard_action_down, keysym, scan_code); });
}

void miw::KeyboardInputDevice::key_release(std::chrono::nanoseconds event_time, xkb_keysym_t keysym, int32_t scan_code)
{
    enqueue([=](EventBuilder* b) { return b->key_event(event_time, mir_keyboard_action_up, keysym, scan_code); });
}

miw::PointerInputDevice::PointerInputDevice(std::shared_ptr<dispatch::ActionQueue> const& action_queue) :
    GenericInputDevice{InputDeviceInfo{"mouse-device", "mouse-dev-1", DeviceCapability::pointer}, action_queue}
{
}

void miw::PointerInputDevice::pointer_press(
    std::chrono::nanoseconds event_time,
    int button,
    geom::PointF const& pos,
    geom::DisplacementF const& scroll)
{
    enqueue([=, this](EventBuilder* b)
    {
        button_state |= to_pointer_button(button, mir_pointer_handedness_right);

        auto const movement = pos - cached_pos;
        cached_pos = pos;
        return b->pointer_event(
            event_time,
            mir_pointer_action_button_down,
            button_state,
            cached_pos.x.as_value(),
            cached_pos.y.as_value(),
            scroll.dx.as_value(),
            scroll.dy.as_value(),
            movement.dx.as_value(),
            movement.dy.as_value()
        );
    });
}

void miw::PointerInputDevice::pointer_release(
    std::chrono::nanoseconds event_time,
    int button,
    geom::PointF const& pos,
    geom::DisplacementF const& scroll)
{
    enqueue([=, this](EventBuilder* b)
    {
        button_state &= ~to_pointer_button(button, mir_pointer_handedness_right);

        auto const movement = pos - cached_pos;
        cached_pos = pos;
        return b->pointer_event(
            event_time,
            mir_pointer_action_button_up,
            button_state,
            cached_pos.x.as_value(),
            cached_pos.y.as_value(),
            scroll.dx.as_value(),
            scroll.dy.as_value(),
            movement.dx.as_value(),
            movement.dy.as_value()
        );
    });
}

void miw::PointerInputDevice::pointer_motion(
    std::chrono::nanoseconds event_time,
    geom::PointF const& pos,
    geom::DisplacementF const& scroll)
{
    enqueue([=, this](EventBuilder* b)
    {
        auto const movement = pos - cached_pos;
        cached_pos = pos;
        return b->pointer_event(
            event_time,
            mir_pointer_action_motion,
            button_state,
            cached_pos.x.as_value(),
            cached_pos.y.as_value(),
            scroll.dx.as_value(),
            scroll.dy.as_value(),
            movement.dx.as_value(),
            movement.dy.as_value()
        );
    });
}

void miw::PointerInputDevice::pointer_axis_motion(
    MirPointerAxisSource pointer_axis_source,
    std::chrono::nanoseconds event_time,
    mir::geometry::PointF const& pos,
    mir::geometry::DisplacementF const& scroll)
{
    enqueue([=, this](EventBuilder* b)
    {
        auto const movement = pos - cached_pos;
        cached_pos = pos;
        return b->pointer_axis_event(
            pointer_axis_source,
            event_time,
            mir_pointer_action_motion,
            button_state,
            cached_pos.x.as_value(),
            cached_pos.y.as_value(),
            scroll.dx.as_value(),
            scroll.dy.as_value(),
            movement.dx.as_value(),
            movement.dy.as_value()
        );
    });
}

miw::TouchInputDevice::TouchInputDevice(std::shared_ptr<dispatch::ActionQueue> const& action_queue) :
    GenericInputDevice{InputDeviceInfo{"touch-device", "touch-dev-1", DeviceCapability::touchscreen}, action_queue}
{
}

void mir::input::wayland::TouchInputDevice::touch_event(
    std::chrono::nanoseconds event_time, std::vector<events::ContactState> const& contacts)
{
    enqueue([=](EventBuilder* b)
    {
        return b->touch_event(event_time, contacts);
    });
}
