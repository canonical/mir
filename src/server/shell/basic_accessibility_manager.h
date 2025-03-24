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

#ifndef MIR_SHELL_BASIC_ACCESSIBILITY_MANAGER_H
#define MIR_SHELL_BASIC_ACCESSIBILITY_MANAGER_H

#include "mir/shell/accessibility_manager.h"

#include "mir/input/mousekeys_common.h"
#include "mir/geometry/displacement.h"

namespace mir
{
class MainLoop;
namespace input
{
class InputEventTransformer;
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
class BasicAccessibilityManager : public AccessibilityManager
{
public:
    BasicAccessibilityManager(
        std::shared_ptr<MainLoop> const& main_loop,
        std::shared_ptr<input::InputEventTransformer> const&,
        std::shared_ptr<time::Clock> const& clock,
        bool enable_key_repeat);

    void register_keyboard_helper(std::shared_ptr<shell::KeyboardHelper> const&) override;

    std::optional<int> repeat_rate() const override;
    int repeat_delay() const override;
    void repeat_rate(int new_rate) override;
    void repeat_delay(int new_rate) override;

    void notify_helpers() const override;

    void set_mousekeys_enabled(bool on) override;
    void set_mousekeys_keymap(input::MouseKeysKeymap const& new_keymap) override;
    void set_acceleration_factors(double constant, double linear, double quadratic) override;
    void set_max_speed(double x_axis, double y_axis) override;

private:
    std::vector<std::shared_ptr<shell::KeyboardHelper>> keyboard_helpers;

    // 25 rate and 600 delay are the default in Weston and Sway
    int repeat_rate_{25};
    int repeat_delay_{600};
    bool enable_key_repeat;

    std::shared_ptr<mir::input::InputEventTransformer> const event_transformer;
    std::shared_ptr<mir::MainLoop> const main_loop;
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
