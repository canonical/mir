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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER

#include "fake_input_device_impl.h"
#include "stub_input_platform.h"

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"
#include "mir/input/input_sink.h"
#include "mir/dispatch/action_queue.h"
#include "mir/module_deleter.h"

#include "boost/throw_exception.hpp"

#include "linux/input.h"

#include "mir_toolkit/events/event.h"

#include <cstring>

namespace mi = mir::input;
namespace md = mir::dispatch;
namespace mtf = mir_test_framework;

namespace
{
const int32_t android_source_keyboard = 0x101;

uint32_t to_modifier(int32_t scan_code)
{
    switch(scan_code)
    {
    case KEY_LEFTALT:
        return mir_key_modifier_alt_left;
    case KEY_RIGHTALT:
        return mir_key_modifier_alt_right;
    case KEY_RIGHTCTRL:
        return mir_key_modifier_ctrl_right;
    case KEY_LEFTCTRL:
        return mir_key_modifier_ctrl_left;
    case KEY_CAPSLOCK:
        return mir_key_modifier_caps_lock;
    case KEY_LEFTMETA:
        return mir_key_modifier_meta_left;
    case KEY_RIGHTMETA:
        return mir_key_modifier_meta_right;
    case KEY_SCROLLLOCK:
        return mir_key_modifier_scroll_lock;
    case KEY_NUMLOCK:
        return mir_key_modifier_num_lock;
    case KEY_LEFTSHIFT:
        return mir_key_modifier_shift_left;
    case KEY_RIGHTSHIFT:
        return mir_key_modifier_shift_right;
    default:
        return mir_key_modifier_none;
    }
}

uint32_t expand_modifier(uint32_t modifiers)
{
    if ((modifiers&mir_key_modifier_alt_left) ||
        (modifiers&mir_key_modifier_alt_right))
        modifiers |= mir_key_modifier_alt;

    if ((modifiers&mir_key_modifier_ctrl_left) ||
        (modifiers&mir_key_modifier_ctrl_right))
        modifiers |= mir_key_modifier_ctrl;

    if ((modifiers&mir_key_modifier_shift_left) ||
        (modifiers&mir_key_modifier_shift_right))
        modifiers |= mir_key_modifier_shift;

    if ((modifiers&mir_key_modifier_meta_left) ||
        (modifiers&mir_key_modifier_meta_right))
        modifiers |= mir_key_modifier_meta;

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
    : info(info), dispatchable{dispatchable}
{
}

void mtf::FakeInputDeviceImpl::InputDevice::synthesize_events(synthesis::KeyParameters const& key_params)
{
    MirEvent key_event;
    std::memset(&key_event, 0, sizeof key_event);
    key_event.type = mir_event_type_key;
    auto event_time = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
    key_event.key.modifiers = expand_modifier(modifiers);
    if (key_params.action == synthesis::EventAction::Down)
    {
        down_times[key_params.scancode] = event_time;
        key_event.key.action = mir_key_action_down;
        modifiers |= to_modifier(key_params.scancode);

        key_event.key.down_time = event_time.count();
    }
    else
    {
        key_event.key.down_time = down_times[key_params.scancode].count();

        down_times.erase(key_params.scancode);
        key_event.key.action = mir_key_action_up;
        modifiers &= ~to_modifier(key_params.scancode);
    }
    key_event.key.source_id = android_source_keyboard;
    key_event.key.flags = MirKeyFlag(0);
    key_event.key.scan_code = key_params.scancode;
    key_event.key.repeat_count = 0;
    key_event.key.event_time = event_time.count();
    key_event.key.is_system_key = 0;

    if (!sink)
        BOOST_THROW_EXCEPTION(std::runtime_error("Device is not started."));
    sink->handle_input(key_event);
}

std::shared_ptr<md::Dispatchable> mtf::FakeInputDeviceImpl::InputDevice::get_dispatchable()
{
    return dispatchable;
}

void mtf::FakeInputDeviceImpl::InputDevice::start(mi::InputSink& destination)
{
    sink = &destination;
}

void mtf::FakeInputDeviceImpl::InputDevice::stop()
{
    sink = nullptr;
}
