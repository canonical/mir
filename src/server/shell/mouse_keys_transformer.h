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

struct MouseKeysTransformer: public mir::input::InputEventTransformer::Transformer
{
    using Dispatcher = mir::input::InputEventTransformer::EventDispatcher;

    MouseKeysTransformer(
        std::shared_ptr<mir::MainLoop> const& main_loop, std::shared_ptr<mir::options::Option> const& options);
    bool transform_input_event(
        mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
        mir::input::EventBuilder* builder,
        MirEvent const& event) override;
    bool handle_motion(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
    bool handle_click(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
    bool handle_change_pointer_button(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
    bool handle_double_click(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
    bool handle_drag_start(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
    bool handle_drag_end(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);

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
        // Quadratic, linear, and constant factors repsectively
        // ax^2 + bx + c
        double const a, b, c;

        AccelerationCurve(std::shared_ptr<mir::options::Option> const& options);

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

    void press_current_cursor_button(Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
    void release_current_cursor_button(Dispatcher const& dispatcher, mir::input::EventBuilder* const builder);
};
}
}
