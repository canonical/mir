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
#include "mir/input/mousekeys_keymap.h"
#include "mir/synchronised.h"

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
class Clock;
}
namespace shell
{
class MouseKeysTransformer: public mir::input::InputEventTransformer::Transformer
{
public:
    virtual void keymap(mir::input::MouseKeysKeymap const& new_keymap) = 0;
    virtual void acceleration_factors(double constant, double linear, double quadratic) = 0;
    virtual void max_speed(double x_axis, double y_axis) = 0;
};

class BasicMouseKeysTransformer: public MouseKeysTransformer
{
public:
    BasicMouseKeysTransformer(
        std::shared_ptr<mir::MainLoop> const& main_loop, std::shared_ptr<time::Clock> const& clock);

    bool transform_input_event(
        mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
        mir::input::EventBuilder* builder,
        MirEvent const& event) override;

    void keymap(mir::input::MouseKeysKeymap const& new_keymap) override;
    void acceleration_factors(double constant, double linear, double quadratic) override;
    void max_speed(double x_axis, double y_axis) override;

private:
    using Dispatcher = mir::input::InputEventTransformer::EventDispatcher;

    bool handle_motion(
        MirKeyboardAction keyboard_action,
        mir::input::MouseKeysKeymap::Action mousekey_action,
        Dispatcher const& dispatcher,
        mir::input::EventBuilder* const builder);

    bool handle_click(
        MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

    bool handle_change_pointer_button(
        MirKeyboardAction keyboard_action,
        mir::input::MouseKeysKeymap::Action mousekeys_action,
        Dispatcher const& dispatcher,
        mir::input::EventBuilder* const builder);

    bool handle_double_click(
        MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

    bool handle_drag(
        MirKeyboardAction keyboard_action,
        mir::input::MouseKeysKeymap::Action mousekeys_action,
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
        double quadratic_factor, linear_factor, constant_factor;

        AccelerationCurve(double quadratic_factor, double linear_factor, double constant_factor);

        double evaluate(double t) const;
    };

    std::shared_ptr<mir::MainLoop> const main_loop;
    std::shared_ptr<time::Clock> const clock;

    // shared_ptr as opposed to unique_ptr like its siblings so we can get a
    // weak ptr to it
    std::shared_ptr<mir::time::Alarm> motion_event_generator;

    struct State {
        MirPointerButtons current_button{mir_pointer_button_primary};
        uint32_t buttons_down{directional_buttons_none};
        bool is_dragging{false};
        AccelerationCurve acceleration_curve{30, 100, 100};
        geometry::DisplacementF max_speed_{400, 400};
        mir::input::MouseKeysKeymap keymap_;
    };

    mir::Synchronised<State> state;
};
}
}
