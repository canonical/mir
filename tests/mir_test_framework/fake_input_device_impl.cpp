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
#include "mir/dispatch/action_queue.h"
#include "mir/geometry/displacement.h"
#include "mir/module_deleter.h"

#include "boost/throw_exception.hpp"
#include "linux/input.h"

#include "mir/events/event_builders.h"

#include <chrono>

namespace mi = mir::input;
namespace md = mir::dispatch;
namespace mtf = mir_test_framework;

namespace
{
const int64_t device_id_unknown = 0;

uint32_t to_modifier(int32_t scan_code)
{
    switch(scan_code)
    {
    case KEY_LEFTALT:
        return mir_input_event_modifier_alt_left;
    case KEY_RIGHTALT:
        return mir_input_event_modifier_alt_right;
    case KEY_RIGHTCTRL:
        return mir_input_event_modifier_ctrl_right;
    case KEY_LEFTCTRL:
        return mir_input_event_modifier_ctrl_left;
    case KEY_CAPSLOCK:
        return mir_input_event_modifier_caps_lock;
    case KEY_LEFTMETA:
        return mir_input_event_modifier_meta_left;
    case KEY_RIGHTMETA:
        return mir_input_event_modifier_meta_right;
    case KEY_SCROLLLOCK:
        return mir_input_event_modifier_scroll_lock;
    case KEY_NUMLOCK:
        return mir_input_event_modifier_num_lock;
    case KEY_LEFTSHIFT:
        return mir_input_event_modifier_shift_left;
    case KEY_RIGHTSHIFT:
        return mir_input_event_modifier_shift_right;
    default:
        return mir_input_event_modifier_none;
    }
}

uint32_t expand_modifier(uint32_t modifiers)
{
    if ((modifiers&mir_input_event_modifier_alt_left) ||
        (modifiers&mir_input_event_modifier_alt_right))
        modifiers |= mir_input_event_modifier_alt;

    if ((modifiers&mir_input_event_modifier_ctrl_left) ||
        (modifiers&mir_input_event_modifier_ctrl_right))
        modifiers |= mir_input_event_modifier_ctrl;

    if ((modifiers&mir_input_event_modifier_shift_left) ||
        (modifiers&mir_input_event_modifier_shift_right))
        modifiers |= mir_input_event_modifier_shift;

    if ((modifiers&mir_input_event_modifier_meta_left) ||
        (modifiers&mir_input_event_modifier_meta_right))
        modifiers |= mir_input_event_modifier_meta;

    return modifiers;
}

}

mtf::FakeInputDeviceImpl::FakeInputDeviceImpl(mi::InputDeviceInfo const& info)
    : queue{mir::make_module_ptr<md::ActionQueue>()},
    device{mir::make_module_ptr<InputDevice>(info, queue)}
{
    mtf::StubInputPlatform::add(device);
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
    : info(info), queue{dispatchable}
{
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::KeyParameters const& key_params)
{
    xkb_keysym_t key_code = 0;
    int64_t event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::system_clock::now().time_since_epoch()).count();
    auto input_action = (key_params.action == synthesis::EventAction::Down) ? mir_key_input_event_action_down :
                                                                              mir_key_input_event_action_up;

    auto event_modifiers = expand_modifier(modifiers);
    auto key_event = mir::events::make_event(
        device_id_unknown, event_time, input_action, key_code, key_params.scancode, event_modifiers);

    if (key_params.action == synthesis::EventAction::Down)
        modifiers |= to_modifier(key_params.scancode);
    else
        modifiers &= ~to_modifier(key_params.scancode);

    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));
    sink->handle_input(*key_event);
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::ButtonParameters const& button)
{
    int64_t event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::system_clock::now().time_since_epoch()).count();
    auto action = update_buttons(button.action, button.button);
    auto event_modifiers = expand_modifier(modifiers);
    auto button_event = mir::events::make_event(device_id_unknown,
                                                event_time,
                                                event_modifiers,
                                                action,
                                                buttons,
                                                pos.x.as_float(),
                                                pos.y.as_float(),
                                                scroll.x.as_float(),
                                                scroll.y.as_float());

    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));
    sink->handle_input(*button_event);
}

MirPointerInputEventAction mtf::FakeInputDeviceImpl::InputDevice::update_buttons(synthesis::EventAction action, int button)
{
    if (action == synthesis::EventAction::Down)
    {
        buttons.push_back(static_cast<MirPointerInputEventButton>(button));
        return mir_pointer_input_event_action_button_down;
    }
    else
    {
        buttons.erase(remove(begin(buttons), end(buttons), static_cast<MirPointerInputEventButton>(button)));
        return mir_pointer_input_event_action_button_up;
    }
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::MotionParameters const& pointer)
{
    int64_t event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::system_clock::now().time_since_epoch()).count();
    auto event_modifiers = expand_modifier(modifiers);
    update_position(pointer.rel_x, pointer.rel_y);
    auto pointer_event = mir::events::make_event(device_id_unknown,
                                                event_time,
                                                event_modifiers,
                                                mir_pointer_input_event_action_motion,
                                                buttons,
                                                pos.x.as_float(),
                                                pos.y.as_float(),
                                                scroll.x.as_float(),
                                                scroll.y.as_float());

    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));
    sink->handle_input(*pointer_event);
}

void mtf::FakeInputDeviceImpl::InputDevice::update_position(int rel_x, int rel_y)
{
    // confine to rectangle of screen
    pos = pos + mir::geometry::Displacement{rel_x, rel_y};
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::TouchParameters const& touch)
{
    int64_t event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::system_clock::now().time_since_epoch()).count();
    auto event_modifiers = expand_modifier(modifiers);

    auto touch_event = mir::events::make_event(device_id_unknown,
                                               event_time,
                                               event_modifiers);

    auto touch_action = mir_touch_input_event_action_up;
    if (touch.action == synthesis::TouchParameters::Action::Tap)
        touch_action = mir_touch_input_event_action_down;
    else if(touch.action == synthesis::TouchParameters::Action::Move)
        touch_action = mir_touch_input_event_action_change;

    MirTouchInputEventTouchId touch_id = 1;
    float pressure = 100.0f;
    float touch_major = 5.0f;
    float touch_minor = 8.0f;
    float size_value = 8.0f;

    mir::events::add_touch(*touch_event,
                           touch_id,
                           touch_action,
                           mir_touch_input_tool_type_finger,
                           float(touch.abs_x),
                           float(touch.abs_y),
                           pressure,
                           touch_major,
                           touch_minor,
                           size_value);

    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));
    sink->handle_input(*touch_event);
}

std::shared_ptr<md::Dispatchable> mtf::FakeInputDeviceImpl::InputDevice::dispatchable()
{
    return queue;
}

void mtf::FakeInputDeviceImpl::InputDevice::start(mi::InputSink* destination)
{
    sink = destination;
}

void mtf::FakeInputDeviceImpl::InputDevice::stop()
{
    sink = nullptr;
}
