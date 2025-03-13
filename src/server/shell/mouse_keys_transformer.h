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

#include "mir/input/input_event_transformer.h"

#include "mir/geometry/displacement.h"
#include <unordered_map>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace mir
{
namespace options
{
class Option;
}
namespace time
{
class Alarm;
}
namespace input
{

class MouseKeysTransformer: public mir::input::InputEventTransformer::Transformer
{
public:
    // Quadratic, linear, and constant factors repsectively
    // ax^2 + bx + c
    struct AccelerationParameters { double const a, b, c; };

    enum class Action
    {
        move_left,
        move_right,
        move_up,
        move_down,
        click,
        double_click,
        drag_start,
        drag_end,
        button_primary,
        button_secondary,
        button_tertiary
    };

    using XkbSymkey = unsigned int;
    using Keymap = std::unordered_map<XkbSymkey, Action>;

    // Qualifier salad
    auto static inline const default_keymap = Keymap{
        {XKB_KEY_KP_2, Action::move_down},
        {XKB_KEY_KP_4, Action::move_left},
        {XKB_KEY_KP_6, Action::move_right},
        {XKB_KEY_KP_8, Action::move_up},
        {XKB_KEY_KP_5, Action::click},
        {XKB_KEY_KP_Add, Action::double_click},
        {XKB_KEY_KP_0, Action::drag_start},
        {XKB_KEY_KP_Decimal, Action::drag_end},
        {XKB_KEY_KP_Divide, Action::button_primary},
        {XKB_KEY_KP_Multiply, Action::button_tertiary},
        {XKB_KEY_KP_Subtract, Action::button_secondary},
    };

    MouseKeysTransformer(
        std::shared_ptr<mir::MainLoop> const& main_loop,
        geometry::Displacement max_speed,
        AccelerationParameters const& params,
        Keymap keymap = default_keymap);

    bool transform_input_event(
        mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
        mir::input::EventBuilder* builder,
        MirEvent const& event) override;

private:
    using Dispatcher = mir::input::InputEventTransformer::EventDispatcher;

    bool handle_motion(
        MirKeyboardAction keyboard_action,
        Action mousekey_action,
        Dispatcher const& dispatcher,
        mir::input::EventBuilder* const builder);

    bool handle_click(
        MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

    bool handle_change_pointer_button(
        MirKeyboardAction keyboard_action,
        Action mousekeys_action,
        Dispatcher const& dispatcher,
        mir::input::EventBuilder* const builder);

    bool handle_double_click(
        MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

    bool handle_drag(
        MirKeyboardAction keyboard_action,
        Action mousekeys_action,
        Dispatcher const& dispatcher,
        mir::input::EventBuilder* const builder);

    void press_current_cursor_button(Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
    void release_current_cursor_button(Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

    enum DirectionalButtons
    {
        none = 0,
        up = 1 << 0,
        down = 1 << 1,
        left = 1 << 2,
        right = 1 << 3
    };

    struct AccelerationCurve
    {
        AccelerationParameters params;

        AccelerationCurve(AccelerationParameters const& params);

        double evaluate(double t) const;
    };

    std::shared_ptr<mir::MainLoop> const main_loop;

    std::shared_ptr<mir::time::Alarm> motion_event_generator; // shared_ptr so we can get a weak ptr to it
    std::unique_ptr<mir::time::Alarm> click_event_generator;
    std::unique_ptr<mir::time::Alarm> double_click_event_generator;

    mir::geometry::DisplacementF motion_direction;
    MirPointerButtons current_button{mir_pointer_button_primary};

    uint32_t buttons_down{none};

    AccelerationCurve const acceleration_curve;
    geometry::DisplacementF max_speed;

    bool is_dragging{false};

    Keymap const keymap;
};
}
}
