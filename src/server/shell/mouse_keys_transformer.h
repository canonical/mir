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

#include <memory>
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
class MouseKeysKeymap;
enum class MouseKeysAction;
class MouseKeysTransformer: public mir::input::InputEventTransformer::Transformer
{
public:
    // Quadratic, linear, and constant factors repsectively
    // ax^2 + bx + c
    struct AccelerationParameters { double const a, b, c; };

    MouseKeysTransformer(
        std::shared_ptr<mir::MainLoop> const& main_loop,
        geometry::Displacement max_speed,
        AccelerationParameters const& params);

    bool transform_input_event(
        mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
        mir::input::EventBuilder* builder,
        MirEvent const& event) override;

    void set_keymap(MouseKeysKeymap const& new_keymap);

private:
    using Dispatcher = mir::input::InputEventTransformer::EventDispatcher;

    bool handle_motion(
        MirKeyboardAction keyboard_action,
        MouseKeysAction mousekey_action,
        Dispatcher const& dispatcher,
        mir::input::EventBuilder* const builder);

    bool handle_click(
        MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

    bool handle_change_pointer_button(
        MirKeyboardAction keyboard_action,
        MouseKeysAction mousekeys_action,
        Dispatcher const& dispatcher,
        mir::input::EventBuilder* const builder);

    bool handle_double_click(
        MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

    bool handle_drag(
        MirKeyboardAction keyboard_action,
        MouseKeysAction mousekeys_action,
        Dispatcher const& dispatcher,
        mir::input::EventBuilder* const builder);

    void press_current_cursor_button(Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
    void release_current_cursor_button(Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

    enum DirectionalButtons
    {
        directional_buttons_none = 0,
        directional_buttons_up = 1 << 0,
        directional_buttons_down = 1 << 1,
        directional_buttons_left = 1 << 2,
        directional_buttons_right = 1 << 3
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

    uint32_t buttons_down{directional_buttons_none};

    AccelerationCurve const acceleration_curve;
    geometry::DisplacementF const max_speed;

    bool is_dragging{false};

    std::mutex state_mutex;
    std::shared_ptr<MouseKeysKeymap> keymap;
};
}
}
