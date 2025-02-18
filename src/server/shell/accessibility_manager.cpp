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

#include "mir/shell/accessibility_manager.h"

#include "mir/geometry/displacement.h"
#include "mir/input/event_builder.h"
#include "mir/main_loop.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/shell/keyboard_helper.h"
#include "mir/time/alarm.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>
#include <optional>

void mir::shell::AccessibilityManager::register_keyboard_helper(std::shared_ptr<KeyboardHelper> const& helper)
{
    mutable_state.lock()->keyboard_helpers.push_back(helper);
}

std::optional<int> mir::shell::AccessibilityManager::repeat_rate() const {
    if(!enable_key_repeat)
        return {};
    return mutable_state.lock()->repeat_rate;
}

int mir::shell::AccessibilityManager::repeat_delay() const {
    return mutable_state.lock()->repeat_delay;
}

void mir::shell::AccessibilityManager::repeat_rate(int new_rate) {
    mutable_state.lock()->repeat_rate = new_rate;
}

void mir::shell::AccessibilityManager::repeat_delay(int new_delay) {
    mutable_state.lock()->repeat_delay = new_delay;
}

void mir::shell::AccessibilityManager::notify_helpers() const {
    for(auto const& helper: mutable_state.lock()->keyboard_helpers)
        helper->repeat_info_changed(repeat_rate(), repeat_delay());
}

struct MouseKeysTransformer: public mir::input::InputEventTransformer::Transformer
{
    using Dispatcher = mir::input::InputEventTransformer::EventDispatcher;

    MouseKeysTransformer(std::shared_ptr<mir::MainLoop> const& main_loop, std::shared_ptr<mir::options::Option> const& options) :
        main_loop{main_loop},
        acceleration_curve(options)
    {
    }

    bool transform_input_event(
        mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
        mir::input::EventBuilder* builder,
        MirEvent const& event) override
    {

        if (mir_event_get_type(&event) != mir_event_type_input)
            return false;

        MirInputEvent const* input_event = mir_event_get_input_event(&event);

        if (mir_input_event_get_type(input_event) == mir_input_event_type_key)
        {
            MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);

            if (handle_motion(kev, dispatcher, builder))
                return true;
            if (handle_click(kev, dispatcher, builder))
                return true;
            if (handle_change_pointer_button(kev))
                return true;
            if (handle_double_click(kev, dispatcher, builder))
                return true;
            if (handle_drag_start(kev, dispatcher, builder))
                return true;
            if (handle_drag_end(kev, dispatcher, builder))
                return true;
        }

        return false;
    }

    bool handle_motion(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
    {
        namespace geom = mir::geometry;

        auto const action = mir_keyboard_event_action(kev);
        auto const set_or_clear = [](uint32_t& buttons_down, DirectionalButtons button, MirKeyboardAction action)
        {
            if (action == mir_keyboard_action_down)
                buttons_down |= button;
            else if (action == mir_keyboard_action_up)
                buttons_down &= ~(button);
        };

        switch (mir_keyboard_event_keysym(kev))
        {
        case XKB_KEY_KP_2:
            set_or_clear(buttons_down, down, action);
            break;
        case XKB_KEY_KP_4:
            set_or_clear(buttons_down, left, action);
            break;
        case XKB_KEY_KP_6:
            set_or_clear(buttons_down, right, action);
            break;
        case XKB_KEY_KP_8:
            set_or_clear(buttons_down, up, action);
            break;
        default:
            return false;
        }

        switch (mir_keyboard_event_action(kev))
        {
        case mir_keyboard_action_up:
            if (buttons_down == none)
                motion_event_generator.reset();
            return true;
        case mir_keyboard_action_down:
            {
                if (!motion_event_generator)
                {
                    motion_event_generator = main_loop->create_alarm(
                        [dispatcher, this, builder, motion_start_time = std::chrono::steady_clock::now()] mutable
                        {
                            auto const motion_step_time = std::chrono::steady_clock::now();
                            auto const t =
                                std::chrono::duration_cast<std::chrono::seconds>(motion_step_time - motion_start_time);
                            auto const speed = acceleration_curve.evaluate(t.count());

                            motion_direction = {0, 0};
                            if (buttons_down & up)
                                motion_direction.dy += geom::DeltaYF{-speed};
                            if (buttons_down & down)
                                motion_direction.dy += geom::DeltaYF{speed};
                            if (buttons_down & left)
                                motion_direction.dx += geom::DeltaXF{-speed};
                            if (buttons_down & right)
                                motion_direction.dx += geom::DeltaXF{speed};

                            // If the cursor stops moving without releasing
                            // all buttons (if for example you press two
                            // opposite directions), reset the time for
                            // acceleration.
                            if (motion_direction.length_squared() == 0)
                                motion_start_time = motion_step_time;

                            dispatcher(builder->pointer_event(
                                std::nullopt,
                                mir_pointer_action_motion,
                                0,
                                std::nullopt,
                                motion_direction,
                                mir_pointer_axis_source_none,
                                mir::events::ScrollAxisH{},
                                mir::events::ScrollAxisV{}));

                            motion_event_generator->reschedule_in(std::chrono::milliseconds(2));
                        });

                    motion_event_generator->reschedule_in(std::chrono::milliseconds(0));
                }
            }
            return true;
        case mir_keyboard_action_repeat:
            return true;
        default:
            return false;
        }
    }

    bool handle_click(MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
    {
        auto const action = mir_keyboard_event_action(kev);

        switch (mir_keyboard_event_keysym(kev))
        {
        case XKB_KEY_KP_5:
            switch (action)
            {
            case mir_keyboard_action_down:
                dispatcher(builder->pointer_event(
                    std::nullopt,
                    mir_pointer_action_button_down,
                    current_button,
                    std::nullopt,
                    {0, 0},
                    mir_pointer_axis_source_none,
                    mir::events::ScrollAxisH{},
                    mir::events::ScrollAxisV{}));
                return true;
            case mir_keyboard_action_up:
                {
                    if (click_event_generator)
                        click_event_generator.reset();

                    dispatcher(builder->pointer_event(
                        std::nullopt,
                        mir_pointer_action_button_up,
                        0,
                        std::nullopt,
                        {0, 0},
                        mir_pointer_axis_source_none,
                        mir::events::ScrollAxisH{},
                        mir::events::ScrollAxisV{}));

                    click_event_generator = main_loop->create_alarm(
                        [dispatcher, this, builder]
                        {
                            dispatcher(builder->pointer_event(
                                std::nullopt,
                                mir_pointer_action_button_up,
                                current_button,
                                std::nullopt,
                                {0, 0},
                                mir_pointer_axis_source_none,
                                mir::events::ScrollAxisH{},
                                mir::events::ScrollAxisV{}));
                        });
                    click_event_generator->reschedule_in(std::chrono::milliseconds(100));

                    return true;
                }
            case mir_keyboard_action_repeat:
                return true;
            default:
                return false;
            }
        default:
            break;
        }

        return false;
    }

    bool handle_change_pointer_button(MirKeyboardEvent const* kev)
    {
        auto const repeat_or_down = mir_keyboard_event_action(kev) == mir_keyboard_action_down ||
                                    mir_keyboard_event_action(kev) == mir_keyboard_action_repeat;

        // Up events
        switch (mir_keyboard_event_keysym(kev))
        {
        case XKB_KEY_KP_Divide:
            if (!repeat_or_down)
                current_button = mir_pointer_button_primary;
            return true;
        case XKB_KEY_KP_Multiply:
            if (!repeat_or_down)
                current_button = mir_pointer_button_tertiary;
            return true;
        case XKB_KEY_KP_Subtract:
            if (!repeat_or_down)
                current_button = mir_pointer_button_secondary;
            return true;
        default:
            return false;
        }
    }

    bool handle_double_click(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
    {
        if (mir_keyboard_event_keysym(kev) == XKB_KEY_KP_Add)
        {
            if (mir_keyboard_event_action(kev) == mir_keyboard_action_down ||
                mir_keyboard_event_action(kev) == mir_keyboard_action_repeat)
                return true;

            dispatcher(builder->pointer_event(
                std::nullopt,
                mir_pointer_action_button_down,
                current_button,
                std::nullopt,
                {0, 0},
                mir_pointer_axis_source_none,
                mir::events::ScrollAxisH{},
                mir::events::ScrollAxisV{}));

            dispatcher(builder->pointer_event(
                std::nullopt,
                mir_pointer_action_button_up,
                0,
                std::nullopt,
                {0, 0},
                mir_pointer_axis_source_none,
                mir::events::ScrollAxisH{},
                mir::events::ScrollAxisV{}));

            double_click_event_generator = main_loop->create_alarm(
                [dispatcher, current_button = this->current_button, builder]
                {
                    dispatcher(builder->pointer_event(
                        std::nullopt,
                        mir_pointer_action_button_down,
                        current_button,
                        std::nullopt,
                        {0, 0},
                        mir_pointer_axis_source_none,
                        mir::events::ScrollAxisH{},
                        mir::events::ScrollAxisV{}));

                    dispatcher(builder->pointer_event(
                        std::nullopt,
                        mir_pointer_action_button_up,
                        0,
                        std::nullopt,
                        {0, 0},
                        mir_pointer_axis_source_none,
                        mir::events::ScrollAxisH{},
                        mir::events::ScrollAxisV{}));
                });

            double_click_event_generator->reschedule_in(std::chrono::milliseconds(100));
        }

        return false;
    }

    bool handle_drag_start(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
    {
        if (mir_keyboard_event_keysym(kev) == XKB_KEY_KP_0)
        {
            if (mir_keyboard_event_action(kev) == mir_keyboard_action_down ||
                mir_keyboard_event_action(kev) == mir_keyboard_action_repeat)
                return true;

            dispatcher(builder->pointer_event(
                std::nullopt,
                mir_pointer_action_button_down,
                current_button,
                std::nullopt,
                {0, 0},
                mir_pointer_axis_source_none,
                mir::events::ScrollAxisH{},
                mir::events::ScrollAxisV{}));

            return true;
        }

        return false;
    }

    bool handle_drag_end(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
    {
        if (mir_keyboard_event_keysym(kev) == XKB_KEY_KP_Decimal)
        {
            if (mir_keyboard_event_action(kev) == mir_keyboard_action_down ||
                mir_keyboard_event_action(kev) == mir_keyboard_action_repeat)
                return true;

            dispatcher(builder->pointer_event(
                std::nullopt,
                mir_pointer_action_button_up,
                0,
                std::nullopt,
                {0, 0},
                mir_pointer_axis_source_none,
                mir::events::ScrollAxisH{},
                mir::events::ScrollAxisV{}));

            return true;
        }

        return false;
    }


    std::shared_ptr<mir::MainLoop> const main_loop;

    std::unique_ptr<mir::time::Alarm> motion_event_generator;
    std::unique_ptr<mir::time::Alarm> click_event_generator;
    std::unique_ptr<mir::time::Alarm> double_click_event_generator;

    mir::geometry::DisplacementF motion_direction;
    MirPointerButtons current_button{mir_pointer_button_primary};

    enum DirectionalButtons
    {
        none = 0,
        up = 1 << 0,
        down = 1 << 1,
        left = 1 << 2,
        right = 1 << 3
    };

    uint32_t buttons_down{none};

    struct AccelerationCurve
    {
        // Quadratic, linear, and constant factors repsectively
        // ax^2 + bx + c
        float const a, b, c;

        AccelerationCurve(std::shared_ptr<mir::options::Option> const& options) :
            a(options->get<double>(mir::options::mouse_keys_acceleration_quadratic_factor)),
            b(options->get<double>(mir::options::mouse_keys_acceleration_linear_factor)),
            c(options->get<double>(mir::options::mouse_keys_acceleration_constant_factor))
        {
        }

        float evaluate(float t) const
        {
            return a * t * t + b * t + c;
        }
    };

    AccelerationCurve const acceleration_curve;
};

mir::shell::AccessibilityManager::AccessibilityManager(
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer) :
    enable_key_repeat{options->get<bool>(options::enable_key_repeat_opt)},
    enable_mouse_keys{options->get<bool>(options::enable_mouse_keys_opt)},
    event_transformer{event_transformer},
    transformer{std::make_shared<MouseKeysTransformer>(main_loop, options)}
{
    if (enable_mouse_keys)
        event_transformer->append(transformer);
}
