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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "fake_input_device_impl.h"
#include "stub_input_platform.h"

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/input/input_sink.h"
#include "mir/input/event_builder.h"
#include "mir/dispatch/action_queue.h"
#include "mir/geometry/displacement.h"
#include "mir/module_deleter.h"
#include "src/platforms/evdev/input_modifier_utils.h"

#include "boost/throw_exception.hpp"

#include "mir/events/event_builders.h"

#include <chrono>

namespace mi = mir::input;
namespace mie = mi::evdev;
namespace md = mir::dispatch;
namespace mtf = mir_test_framework;

mtf::FakeInputDeviceImpl::FakeInputDeviceImpl(mi::InputDeviceInfo const& info)
    : queue{mir::make_module_ptr<md::ActionQueue>()}, device{mir::make_module_ptr<InputDevice>(info, queue)}
{
    mtf::StubInputPlatform::add(device);
}

void mtf::FakeInputDeviceImpl::emit_runtime_error()
{
    queue->enqueue([]()
                   {
                       throw std::runtime_error("runtime error in input device");
                   });
}

void mtf::FakeInputDeviceImpl::emit_event(synthesis::KeyParameters const& key)
{
    queue->enqueue([this, key]()
                   {
                       device->synthesize_events(key);
                   });
}

void mtf::FakeInputDeviceImpl::emit_event(synthesis::ButtonParameters const& button)
{
    queue->enqueue([this, button]()
                   {
                       device->synthesize_events(button);
                   });
}

void mtf::FakeInputDeviceImpl::emit_event(synthesis::MotionParameters const& motion)
{
    queue->enqueue([this, motion]()
                   {
                       device->synthesize_events(motion);
                   });
}

void mtf::FakeInputDeviceImpl::emit_event(synthesis::TouchParameters const& touch)
{
    queue->enqueue([this, touch]()
                   {
                       device->synthesize_events(touch);
                   });
}

mtf::FakeInputDeviceImpl::InputDevice::InputDevice(mi::InputDeviceInfo const& info,
                                                   std::shared_ptr<mir::dispatch::Dispatchable> const& dispatchable)
    : info(info), queue{dispatchable}, buttons{0}
{
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::KeyParameters const& key_params)
{
    xkb_keysym_t key_code = 0;

    auto event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch());

    auto input_action =
        (key_params.action == synthesis::EventAction::Down) ? mir_keyboard_action_down : mir_keyboard_action_up;

    auto event_modifiers = mie::expand_modifiers(modifiers);
    auto key_event = builder->key_event(event_time, input_action, key_code, key_params.scancode, event_modifiers);

    if (key_params.action == synthesis::EventAction::Down)
        modifiers |= mie::to_modifier(key_params.scancode);
    else
        modifiers &= ~mie::to_modifier(key_params.scancode);

    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));
    sink->handle_input(*key_event);
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::ButtonParameters const& button)
{
    auto event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    auto action = update_buttons(button.action, mie::to_pointer_button(button.button));
    auto event_modifiers = mie::expand_modifiers(modifiers);
    auto button_event = builder->pointer_event(event_time,
                                               event_modifiers,
                                               action,
                                               buttons,
                                               pos.x.as_float(),
                                               pos.y.as_float(),
                                               scroll.x.as_float(),
                                               scroll.y.as_float(),
                                               0.0f,
                                               0.0f);

    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));
    sink->handle_input(*button_event);
}

MirPointerAction mtf::FakeInputDeviceImpl::InputDevice::update_buttons(synthesis::EventAction action, MirPointerButton button)
{
    if (action == synthesis::EventAction::Down)
    {
        buttons |= button;
        return mir_pointer_action_button_down;
    }
    else
    {
        buttons &= ~button;
        return mir_pointer_action_button_up;
    }
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::MotionParameters const& pointer)
{
    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));

    auto event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    auto event_modifiers = mie::expand_modifiers(modifiers);
    update_position(pointer.rel_x, pointer.rel_y);
    auto pointer_event = builder->pointer_event(event_time,
                                                event_modifiers,
                                                mir_pointer_action_motion,
                                                buttons,
                                                pos.x.as_float(),
                                                pos.y.as_float(),
                                                scroll.x.as_float(),
                                                scroll.y.as_float(),
                                                pointer.rel_x,
                                                pointer.rel_y);

    sink->handle_input(*pointer_event);
}

void mtf::FakeInputDeviceImpl::InputDevice::update_position(int rel_x, int rel_y)
{
    pos = pos + mir::geometry::Displacement{rel_x, rel_y};
    sink->confine_pointer(pos);
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::TouchParameters const& touch)
{
    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));

    auto event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch());
    auto event_modifiers = mie::expand_modifiers(modifiers);

    auto touch_event = builder->touch_event(event_time, event_modifiers);

    auto touch_action = mir_touch_action_up;
    if (touch.action == synthesis::TouchParameters::Action::Tap)
        touch_action = mir_touch_action_down;
    else if (touch.action == synthesis::TouchParameters::Action::Move)
        touch_action = mir_touch_action_change;

    MirTouchId touch_id = 1;
    float pressure = 1.0f;

    float abs_x = touch.abs_x;
    float abs_y = touch.abs_y;
    map_touch_coordinates(abs_x, abs_y);
    // those values would need scaling too as soon as they can be controlled by the caller
    float touch_major = 5.0f;
    float touch_minor = 8.0f;
    float size_value = 8.0f;

    builder->add_touch(*touch_event,
                       touch_id,
                       touch_action,
                       mir_touch_tooltype_finger,
                       abs_x,
                       abs_y,
                       pressure,
                       touch_major,
                       touch_minor,
                       size_value);

    sink->handle_input(*touch_event);
}

void mtf::FakeInputDeviceImpl::InputDevice::map_touch_coordinates(float& x, float& y)
{
    // TODO take orientation of input sink into account?
    auto area = sink->bounding_rectangle();
    auto touch_range = FakeInputDevice::maximum_touch_axis_value - FakeInputDevice::minimum_touch_axis_value + 1;
    auto x_scale = area.size.width.as_float() / float(touch_range);
    auto y_scale = area.size.height.as_float() / float(touch_range);
    x = (x - float(FakeInputDevice::minimum_touch_axis_value))*x_scale + area.top_left.x.as_float();
    y = (y - float(FakeInputDevice::minimum_touch_axis_value))*y_scale + area.top_left.y.as_float();
}

std::shared_ptr<md::Dispatchable> mtf::FakeInputDeviceImpl::InputDevice::dispatchable()
{
    return queue;
}

void mtf::FakeInputDeviceImpl::InputDevice::start(mi::InputSink* destination, mi::EventBuilder* event_builder)
{
    sink = destination;
    builder = event_builder;
}

void mtf::FakeInputDeviceImpl::InputDevice::stop()
{
    sink = nullptr;
    builder = nullptr;
}
