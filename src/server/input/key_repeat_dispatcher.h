/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_KEY_REPEAT_DISPATCHER_H_
#define MIR_INPUT_KEY_REPEAT_DISPATCHER_H_

#include "mir/input/input_dispatcher.h"
#include "mir/input/input_device_observer.h"
#include "mir/optional_value.h"

#include <memory>
#include <chrono>
#include <mutex>
#include <unordered_map>

namespace mir
{
namespace cookie
{
class Authority;
}
namespace time
{
class AlarmFactory;
class Alarm;
}
namespace input
{
class InputDeviceHub;
class KeyRepeatDispatcher : public InputDispatcher
{
public:
    KeyRepeatDispatcher(std::shared_ptr<InputDispatcher> const& next_dispatcher,
                        std::shared_ptr<time::AlarmFactory> const& factory,
                        std::shared_ptr<cookie::Authority> const& cookie_authority,
                        bool repeat_enabled,
                        std::chrono::milliseconds repeat_timeout, /* timeout before sending first repeat */
                        std::chrono::milliseconds repeat_delay, /* delay between repeated keys */
                        bool disable_repeat_on_touchscreen);

    // InputDispatcher
    bool dispatch(std::shared_ptr<MirEvent const> const& event) override;
    void start() override;
    void stop() override;

    void set_input_device_hub(std::shared_ptr<InputDeviceHub> const& hub);

    void set_touch_button_device(MirInputDeviceId id);
    void remove_device(MirInputDeviceId id);
private:
    std::mutex repeat_state_mutex;

    std::shared_ptr<InputDispatcher> const next_dispatcher;
    std::shared_ptr<time::AlarmFactory> const alarm_factory;
    std::shared_ptr<cookie::Authority> const cookie_authority;
    bool const repeat_enabled;
    std::chrono::milliseconds repeat_timeout;
    std::chrono::milliseconds const repeat_delay;
    bool const disable_repeat_on_touchscreen;
    optional_value<MirInputDeviceId> touch_button_device;

    struct KeyboardState
    {
        std::unordered_map<int, std::shared_ptr<mir::time::Alarm>> repeat_alarms_by_scancode;
    };
    std::unordered_map<MirInputDeviceId, KeyboardState> repeat_state_by_device;
    KeyboardState& ensure_state_for_device_locked(std::lock_guard<std::mutex> const&, MirInputDeviceId id);

    bool handle_key_input(MirInputDeviceId id, MirKeyboardEvent const* ev);
};

}
}

#endif // MIR_INPUT_KEY_REPEAT_DISPATCHER_H_
