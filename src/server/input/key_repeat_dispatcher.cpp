/*
 * Copyright © Canonical Ltd.
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
 */

#include "key_repeat_dispatcher.h"

#include "mir/input/device.h"
#include "mir/input/input_device_hub.h"
#include "mir/time/alarm_factory.h"
#include "mir/time/alarm.h"
#include "mir/events/event_builders.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <boost/throw_exception.hpp>

#include <stdexcept>
#include <chrono>

namespace mi = mir::input;
namespace mev = mir::events;

namespace
{
struct DeviceRemovalFilter : mi::InputDeviceObserver
{
    DeviceRemovalFilter(mi::KeyRepeatDispatcher* dispatcher)
        : dispatcher{dispatcher} {}

    void device_added(std::shared_ptr<mi::Device> const& device) override
    {
        if (device->name() == "mtk-tpd")
        {
            dispatcher->set_touch_button_device(device->id());
        }

    }

    void device_changed(std::shared_ptr<mi::Device> const&) override
    {
    }

    void device_removed(std::shared_ptr<mi::Device> const& device) override
    {
        dispatcher->remove_device(device->id());
    }

    void changes_complete() override
    {
    }
    mi::KeyRepeatDispatcher* dispatcher;
};

auto is_meta_key(MirKeyboardEvent const* event) -> bool
{
    auto const keysym = mir_keyboard_event_keysym(event);
    switch(keysym)
    {
    case XKB_KEY_Shift_R:
    case XKB_KEY_Shift_L:
    case XKB_KEY_Alt_R:
    case XKB_KEY_Alt_L:
    case XKB_KEY_Control_R:
    case XKB_KEY_Control_L:
    case XKB_KEY_Super_L:
    case XKB_KEY_Super_R:
    case XKB_KEY_Caps_Lock:
    case XKB_KEY_Scroll_Lock:
    case XKB_KEY_Num_Lock: return true;
    default: return ((XKB_KEY_Shift_L <= keysym) && (keysym <= XKB_KEY_Hyper_R));
    }
}
}

mi::KeyRepeatDispatcher::KeyRepeatDispatcher(
    std::shared_ptr<mi::InputDispatcher> const& next_dispatcher,
    std::shared_ptr<mir::time::AlarmFactory> const& factory,
    bool repeat_enabled,
    std::chrono::milliseconds repeat_timeout,
    std::chrono::milliseconds repeat_delay,
    bool disable_repeat_on_touchscreen)
    : next_dispatcher(next_dispatcher),
      alarm_factory(factory),
      repeat_enabled(repeat_enabled),
      repeat_timeout(repeat_timeout),
      repeat_delay(repeat_delay),
      disable_repeat_on_touchscreen(disable_repeat_on_touchscreen)
{
}

void mi::KeyRepeatDispatcher::set_input_device_hub(std::shared_ptr<InputDeviceHub> const& hub)
{
    hub->add_observer(std::make_shared<DeviceRemovalFilter>(this));
}

void mi::KeyRepeatDispatcher::set_touch_button_device(MirInputDeviceId id)
{
    std::lock_guard lock(repeat_state_mutex);
    touch_button_device = id;
}

void mi::KeyRepeatDispatcher::remove_device(MirInputDeviceId id)
{
    std::lock_guard lock(repeat_state_mutex);
    repeat_state_by_device.erase(id); // destructor cancels alarms
    if (touch_button_device.is_set() && touch_button_device.value() == id)
        touch_button_device.consume();
}

void mi::KeyRepeatDispatcher::set_alarm_for_device(
    MirInputDeviceId id,
    std::shared_ptr<mir::time::Alarm> repeat_alarm)
{
    {
        std::lock_guard lg(repeat_state_mutex);
        // Get the current KeyboardState or insert a new one
        auto const& iter = repeat_state_by_device.insert(std::make_pair(id, KeyboardState()));
        // Set the new alarm
        std::swap(repeat_alarm, iter.first->second.repeat_alarm);
    }
    // Reset the old alarm outside the mutex lock
    repeat_alarm.reset();
}

bool mi::KeyRepeatDispatcher::dispatch(std::shared_ptr<MirEvent const> const& event)
{
    if (!repeat_enabled) // if we made this mutable we'd need a guard
    {
	return next_dispatcher->dispatch(event);
    }

    if (mir_event_get_type(event.get()) == mir_event_type_input)
    {
        auto iev = mir_event_get_input_event(event.get());
        if (mir_input_event_get_type(iev) != mir_input_event_type_key)
            return next_dispatcher->dispatch(event);
        auto device_id = mir_input_event_get_device_id(iev);
        if (!disable_repeat_on_touchscreen || !touch_button_device.is_set() || device_id != touch_button_device.value())
            handle_key_input(mir_input_event_get_device_id(iev), mir_input_event_get_keyboard_event(iev));

        return next_dispatcher->dispatch(event);
    }
    return next_dispatcher->dispatch(event);
}

void mi::KeyRepeatDispatcher::handle_key_input(MirInputDeviceId id, MirKeyboardEvent const* kev)
{
    auto scan_code = mir_keyboard_event_scan_code(kev);

    switch (mir_keyboard_event_action(kev))
    {
    case mir_keyboard_action_up:
    {
        set_alarm_for_device(id, nullptr);
        break;
    }
    case mir_keyboard_action_down:
    {
        // We don't want to track and auto-repeat individual meta key presses
        // That leads, for example, to alternating Ctrl and Alt repeats when
        // both keys are pressed.
        if (is_meta_key(kev))
        {
            // Further, we don't want to repeat with the old modifier state.
            // So just cancel any existing repeats and carry on.
            set_alarm_for_device(id, nullptr);
            return;
        }

        auto clone_event = [scan_code, id,
             next_dispatcher=next_dispatcher,
             keysym = mir_keyboard_event_keysym(kev),
             modifiers = mir_keyboard_event_modifiers(kev)]()
             {
                 auto const now = std::chrono::steady_clock::now().time_since_epoch();
                 std::vector<uint8_t> cookie_vec;
                 auto new_event = mev::make_key_event(
                     id,
                     now,
                     cookie_vec,
                     mir_keyboard_action_repeat,
                     keysym,
                     scan_code,
                     modifiers);
                 next_dispatcher->dispatch(std::move(new_event));
             };

        // We need to provide the alarm lambda with the alarm (which doesn't exist yet) so
        // that it can reschedule. This is a placeholder while we create the lambda & alarm.
        auto const shared_weak_alarm = std::make_shared<std::weak_ptr<time::Alarm>>();

        std::shared_ptr<mir::time::Alarm> alarm = alarm_factory->create_alarm(
            [clone_event, shared_weak_alarm, repeat_delay=repeat_delay]()
            {
                clone_event();

                if (auto const& repeat_alarm = shared_weak_alarm->lock())
                    repeat_alarm->reschedule_in(repeat_delay);
            });

        // Fulfill the placeholder before scheduling the alarm.
        *shared_weak_alarm = alarm;
        alarm->reschedule_in(repeat_timeout);
        set_alarm_for_device(id, alarm);
    }
    case mir_keyboard_action_repeat:
        // Should we consume existing repeats?
        break;
    case mir_keyboard_action_modifiers:
        break;
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Unexpected key event action"));
    }
}

void mi::KeyRepeatDispatcher::start()
{
    next_dispatcher->start();
}

void mi::KeyRepeatDispatcher::stop()
{
    std::lock_guard lg(repeat_state_mutex);

    repeat_state_by_device.clear();

    next_dispatcher->stop();
}
