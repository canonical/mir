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

#include "mir/input/input_event_transformer.h"
#include "mir/shell/accessibility_manager.h"

#include "mir/input/mousekeys_keymap.h"
#include "mir/synchronised.h"

#include <atomic>

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
class MouseKeysTransformer;
class BasicAccessibilityManager : public AccessibilityManager
{
public:
    BasicAccessibilityManager(
        std::shared_ptr<input::InputEventTransformer> const& event_transformer,
        bool enable_key_repeat,
        std::shared_ptr<mir::graphics::Cursor> const& cursor,
        std::shared_ptr<shell::MouseKeysTransformer> const& mousekeys_transformer,
        std::shared_ptr<input::InputEventTransformer::Transformer> const& simulated_secondary_click_transformer);
    ~BasicAccessibilityManager();

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
    struct MutableState {
        // 25 rate and 600 delay are the default in Weston and Sway
        int repeat_rate{25};
        int repeat_delay{600};

        std::vector<std::shared_ptr<shell::KeyboardHelper>> keyboard_helpers;
    };

    template <typename Transformer> class Registration
    {
    public:
        Registration(
            std::shared_ptr<Transformer> const& transformer,
            std::shared_ptr<input::InputEventTransformer> const& event_transformer);

        void add_registration() const;
        void remove_registration() const;

        Transformer* operator->() const noexcept;

    private:
        Registration(Registration const&) = delete;
        Registration& operator=(Registration const&) = delete;

        std::shared_ptr<Transformer> const transformer;
        std::shared_ptr<input::InputEventTransformer> const event_transformer;
        std::atomic<bool> mutable registered{false};
    };

    Synchronised<MutableState> mutable_state;

    bool const enable_key_repeat;
    std::shared_ptr<graphics::Cursor> const cursor;
    Registration<MouseKeysTransformer> const mouse_keys_transformer;
    Registration<input::InputEventTransformer::Transformer> const simulated_secondary_click_transformer;
};
}
}

#endif
