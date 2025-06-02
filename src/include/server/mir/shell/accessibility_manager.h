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

#include <memory>
#include <optional>

namespace mir
{
namespace input
{
class MouseKeysKeymap;
}
namespace shell
{
class KeyboardHelper;
class SimulatedSecondaryClickTransformer;
class AccessibilityManager
{
public:
    virtual ~AccessibilityManager() = default;
    virtual void register_keyboard_helper(std::shared_ptr<shell::KeyboardHelper> const&) = 0;

    virtual std::optional<int> repeat_rate() const = 0;
    virtual int repeat_delay() const = 0;

    // Setting one option to `std::nullopt` signals that it didn't change.
    virtual void repeat_rate_and_delay(std::optional<int> new_rate, std::optional<int> new_delay) = 0;

    virtual void cursor_scale(float new_scale) = 0;

    virtual void mousekeys_enabled(bool on) = 0;
    virtual void mousekeys_keymap(input::MouseKeysKeymap const& new_keymap) = 0;
    virtual void acceleration_factors(double constant, double linear, double quadratic) = 0;
    virtual void max_speed(double x_axis, double y_axis) = 0;

    virtual void simulated_secondary_click_enabled(bool enabled) = 0;
    virtual auto simulated_secondary_click() -> SimulatedSecondaryClickTransformer& = 0;
};
}
}

#endif
