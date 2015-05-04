/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "key_repeat_dispatcher.h"

#include "mir/time/alarm_factory.h"
#include "mir/time/alarm.h"
#include "mir/events/event_private.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <string.h>

namespace mi = mir::input;

mi::KeyRepeatDispatcher::KeyRepeatDispatcher(
    std::shared_ptr<mi::InputDispatcher> const& next_dispatcher,
    std::shared_ptr<mir::time::AlarmFactory> const& factory,
    std::chrono::milliseconds repeat_timeout)
    : next_dispatcher(next_dispatcher),
      alarm_factory(factory),
      repeat_timeout(repeat_timeout)
{
}

mi::KeyRepeatDispatcher::KeyboardState& mi::KeyRepeatDispatcher::ensure_state_for_device_locked(std::lock_guard<std::mutex> const&, MirInputDeviceId id)
{
    repeat_state_by_device.insert(std::make_pair(id, KeyboardState()));
    return repeat_state_by_device[id];
}

bool mi::KeyRepeatDispatcher::dispatch(MirEvent const& event)
{
    auto iev = mir_event_get_input_event(&event);
    if (mir_input_event_get_type(iev) == mir_input_event_type_key)
        handle_key_input(mir_input_event_get_device_id(iev), mir_input_event_get_keyboard_event(iev));
    return next_dispatcher->dispatch(event);
}

namespace
{
inline std::shared_ptr<MirEvent> copy_to_repeat_ev(MirKeyboardEvent const* kev)
{
    auto repeat_ev = std::make_shared<MirEvent>();
    memcpy(repeat_ev.get(), kev, sizeof(MirEvent));
    repeat_ev->key.repeat_count = 1;
    return repeat_ev;
}
}

void mi::KeyRepeatDispatcher::handle_key_input(MirInputDeviceId id, MirKeyboardEvent const* kev)
{
    std::lock_guard<std::mutex> lg(repeat_state_mutex);
    auto& device_state = ensure_state_for_device_locked(lg, id);

    auto scan_code = mir_keyboard_event_scan_code(kev);

    switch (mir_keyboard_event_action(kev))
    {
    case mir_keyboard_action_up:
    {
        auto it = device_state.repeat_alarms_by_scancode.find(scan_code);
        if (it == device_state.repeat_alarms_by_scancode.end())
        {
            return;
        }
        it->second->cancel();
        device_state.repeat_alarms_by_scancode.erase(it);
        break;
    }
    case mir_keyboard_action_down:
    {
        auto it = device_state.repeat_alarms_by_scancode.find(scan_code);
        if (it != device_state.repeat_alarms_by_scancode.end())
        {
            return;
        }
        std::shared_ptr<MirEvent> ev = copy_to_repeat_ev(kev);
        auto &capture_alarm = device_state.repeat_alarms_by_scancode[scan_code];
        std::shared_ptr<mir::time::Alarm> alarm = alarm_factory->create_alarm([this, &capture_alarm, ev]()
            {
                std::lock_guard<std::mutex> lg(repeat_state_mutex);

                ev->key.event_time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                next_dispatcher->dispatch(*ev);

                capture_alarm->reschedule_in(repeat_timeout);
            });
        alarm->reschedule_in(repeat_timeout);
        device_state.repeat_alarms_by_scancode[scan_code] = {alarm};
    }
    case mir_keyboard_action_repeat:
        // TODO: Maybe we should consume existing repeats
        break;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Unexpected key event action"));
    }
}

void mi::KeyRepeatDispatcher::cancel_repeats_for_locked(std::lock_guard<std::mutex> const&, MirInputDeviceId id)
{
    auto it = repeat_state_by_device.find(id);
    if (it == repeat_state_by_device.end())
    {
        return;
    }
    for (auto& kv : it->second.repeat_alarms_by_scancode)
    {
        auto& alarm = kv.second;
        alarm->cancel();
    }
    repeat_state_by_device.erase(it);
}

void mi::KeyRepeatDispatcher::start()
{
    next_dispatcher->start();
}

void mi::KeyRepeatDispatcher::stop()
{
    std::lock_guard<std::mutex> lg(repeat_state_mutex);
    
    for (auto kv : repeat_state_by_device)
    {
        cancel_repeats_for_locked(lg, kv.first);
    }
        
    next_dispatcher->stop();
}
