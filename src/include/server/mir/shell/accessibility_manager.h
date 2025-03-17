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

#ifndef MIR_SHELL_ACCESSIBILITY_MANAGER_H
#define MIR_SHELL_ACCESSIBILITY_MANAGER_H

#include "mir/geometry/displacement.h"
#include "mir/input/input_event_transformer.h"
#include "mir/input/mousekeys_common.h"
#include "mir/synchronised.h"

#include <memory>
#include <optional>
#include <vector>

namespace mir
{
class MainLoop;
namespace input
{
class EventFilter;
class CompositeEventFilter;
class MouseKeysTransformer;
}
namespace options
{
class Option;
}
namespace time
{
class Clock;
}
namespace shell
{
class KeyboardHelper;
class AccessibilityManager
{
public:
    AccessibilityManager(
        std::shared_ptr<MainLoop> const& main_loop,
        std::shared_ptr<mir::options::Option> const&,
        std::shared_ptr<input::InputEventTransformer> const&,
        std::shared_ptr<time::Clock> const& clock);

    void register_keyboard_helper(std::shared_ptr<shell::KeyboardHelper> const&);

    std::optional<int> repeat_rate() const;
    int repeat_delay() const;

    void repeat_rate(int new_rate);
    void repeat_delay(int new_rate);

    void notify_helpers() const;

    void set_mousekeys_enabled(bool on);
    void set_mousekeys_keymap(input::MouseKeysKeymap const& new_keymap);
    void set_acceleration_factors(double constant, double linear, double quadratic);
    void set_max_speed(double x_axis, double y_axis);

private:

    struct MutableState {
        // 25 rate and 600 delay are the default in Weston and Sway
        int repeat_rate{25};
        int repeat_delay{600};

        std::vector<std::shared_ptr<shell::KeyboardHelper>> keyboard_helpers;
    };

    Synchronised<MutableState> mutable_state;

    bool const enable_key_repeat{true};

    std::shared_ptr<mir::input::InputEventTransformer> const event_transformer;
    std::shared_ptr<mir::MainLoop> const main_loop;
    std::shared_ptr<mir::options::Option> const options;
    std::shared_ptr<time::Clock> const clock;

    std::shared_ptr<mir::input::MouseKeysTransformer> transformer;

    // Need to be cached in case values are changed, then mousekeys were
    // disabled and re-enabled.
    input::MouseKeysKeymap keymap;
    double acceleration_constant, acceleration_linear, acceleration_quadratic;
    geometry::DisplacementF max_speed;
};
}
}

#endif
