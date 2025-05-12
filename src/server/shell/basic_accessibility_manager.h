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

#include "mir/input/mousekeys_keymap.h"
#include "mir/synchronised.h"

namespace mir
{
class MainLoop;
namespace graphics
{
class Cursor;
}
namespace input
{
class CompositeEventFilter;
class InputEventTransformer;
class InputDeviceRegistry;
}
namespace shell
{
class MouseKeysTransformer;
class CompositeEventFilter;
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
        std::shared_ptr<MainLoop> main_loop,
        std::shared_ptr<input::CompositeEventFilter> the_composite_event_filter,
        std::shared_ptr<input::InputEventTransformer> const& event_transformer,
        bool enable_key_repeat,
        std::shared_ptr<mir::graphics::Cursor> const& cursor,
        std::shared_ptr<shell::MouseKeysTransformer> const& mousekeys_transformer,
        std::shared_ptr<input::InputDeviceRegistry> const& input_device_registry);

    void register_keyboard_helper(std::shared_ptr<shell::KeyboardHelper> const&) override;

    std::optional<int> repeat_rate() const override;
    int repeat_delay() const override;
    void repeat_rate_and_delay(std::optional<int> new_rate, std::optional<int> new_delay) override;

    void cursor_scale(float new_scale) override;

    void mousekeys_enabled(bool on) override;
    void mousekeys_keymap(input::MouseKeysKeymap const& new_keymap) override;
    void acceleration_factors(double constant, double linear, double quadratic) override;
    void max_speed(double x_axis, double y_axis) override;

private:
    class MousekeyPointer;

    struct MutableState {
        // 25 rate and 600 delay are the default in Weston and Sway
        int repeat_rate{25};
        int repeat_delay{600};

        std::vector<std::shared_ptr<shell::KeyboardHelper>> keyboard_helpers;
        std::shared_ptr<MousekeyPointer> mousekey_pointer;
    };

    Synchronised<MutableState> mutable_state;

    std::shared_ptr<MainLoop> const main_loop;
    std::shared_ptr<input::CompositeEventFilter> const the_composite_event_filter;
    bool const enable_key_repeat;
    std::shared_ptr<graphics::Cursor> const cursor;

    std::shared_ptr<mir::input::InputEventTransformer> const event_transformer;
    std::shared_ptr<input::CompositeEventFilter> const composite_filter;

    std::shared_ptr<mir::shell::MouseKeysTransformer> const transformer;
    std::shared_ptr<mir::input::InputDeviceRegistry> const input_device_registry;
};
}
}

#endif
