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

mtf::FakeInputDeviceImpl::InputDevice::InputDevice(mi::InputDeviceInfo const& info,
                                                   std::shared_ptr<mir::dispatch::Dispatchable> const& dispatchable)
    : info(info), queue{dispatchable}
{
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::KeyParameters const& key_params)
{
    int64_t device_id_unknown = 0;
    xkb_keysym_t key_code = 0;
    int64_t event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::system_clock::now().time_since_epoch()).count();
    auto input_action = (key_params.action == synthesis::EventAction::Down) ? mir_keyboard_action_down:
                                                                              mir_keyboard_action_up;

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
