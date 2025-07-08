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

#include "basic_mousekeys_transformer.h"

#include "mir/geometry/forward.h"
#include "mir/input/event_builder.h"
#include "mir/input/mousekeys_keymap.h"
#include "mir/log.h"
#include "mir/main_loop.h"
#include "mir/time/alarm.h"
#include "mir/time/clock.h"
#include "mir_toolkit/events/enums.h"
#include "mir_toolkit/events/input/keyboard_event.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>

using enum mir::input::MouseKeysKeymap::Action;

namespace
{
mir::input::MouseKeysKeymap const default_keymap = {
    {XKB_KEY_KP_2, move_down},
    {XKB_KEY_KP_4, move_left},
    {XKB_KEY_KP_6, move_right},
    {XKB_KEY_KP_8, move_up},
    {XKB_KEY_KP_5, click},
    {XKB_KEY_KP_Add, double_click},
    {XKB_KEY_KP_0, drag_start},
    {XKB_KEY_KP_Decimal, drag_end},
    {XKB_KEY_KP_Divide, button_primary},
    {XKB_KEY_KP_Multiply, button_tertiary},
    {XKB_KEY_KP_Subtract, button_secondary},
};
}

mir::shell::BasicMouseKeysTransformer::BasicMouseKeysTransformer(
    std::shared_ptr<mir::MainLoop> const& main_loop, std::shared_ptr<time::Clock> const& clock) :
    main_loop{main_loop},
    clock{clock}
{
    state.lock()->keymap_ = ::default_keymap;
}

bool mir::shell::BasicMouseKeysTransformer::transform_input_event(
    mir::input::Transformer::EventDispatcher const& dispatcher,
    mir::input::EventBuilder* builder,
    MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;

    MirInputEvent const* input_event = mir_event_get_input_event(&event);

    if (mir_input_event_get_type(input_event) == mir_input_event_type_key)
    {
        MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
        auto const keysym = mir_keyboard_event_keysym(kev);
        auto const keyboard_action = mir_keyboard_event_action(kev);

        if (auto const mousekey_action = state.lock()->keymap_.get_action(keysym))
        {
            switch (*mousekey_action)
            {
            case move_left:
            case move_right:
            case move_up:
            case move_down:
                return handle_motion(keyboard_action, *mousekey_action, dispatcher, builder);
            case click:
                return handle_click(keyboard_action, dispatcher, builder);
            case double_click:
                return handle_double_click(keyboard_action, dispatcher, builder);
            case drag_start:
            case drag_end:
                return handle_drag(keyboard_action, *mousekey_action, dispatcher, builder);
            case button_primary:
            case button_secondary:
            case button_tertiary:
                return handle_change_pointer_button(keyboard_action, *mousekey_action, dispatcher, builder);
            }
        }
    }

    return false;
}

bool mir::shell::BasicMouseKeysTransformer::handle_motion(
    MirKeyboardAction keyboard_action,
    mir::input::MouseKeysKeymap::Action mousekey_action,
    Dispatcher const& dispatcher,
    mir::input::EventBuilder* const builder)
{
    namespace geom = mir::geometry;

    auto const set_or_clear = [](uint32_t& buttons_down, DirectionalButtons button, MirKeyboardAction action)
    {
        if (action == mir_keyboard_action_down)
            buttons_down |= button;
        else if (action == mir_keyboard_action_up)
            buttons_down &= ~(button);
    };

    {
        auto const locked_state = state.lock();
        auto& buttons_down = locked_state->buttons_down;
        switch (mousekey_action)
        {
        case move_down:
            set_or_clear(buttons_down, directional_buttons_down, keyboard_action);
            break;
        case move_left:
            set_or_clear(buttons_down, directional_buttons_left, keyboard_action);
            break;
        case move_right:
            set_or_clear(buttons_down, directional_buttons_right, keyboard_action);
            break;
        case move_up:
            set_or_clear(buttons_down, directional_buttons_up, keyboard_action);
            break;
        default:
            std::unreachable();
        }
    }

    switch (keyboard_action)
    {
    case mir_keyboard_action_up:
        if (state.lock()->buttons_down == directional_buttons_none)
        {
            motion_event_generator.reset();
        }

        return true;
    case mir_keyboard_action_down:
        {
            if (!motion_event_generator)
            {
                auto const shared_weak_alarm = std::make_shared<std::weak_ptr<mir::time::Alarm>>();
                motion_event_generator = main_loop->create_alarm(
                    [dispatcher,
                     this,
                     builder,
                     motion_start_time = clock->now(),
                     weak_self = shared_weak_alarm,
                     motion_direction = mir::geometry::DisplacementF{0, 0}] mutable
                    {
                        using SecondF = std::chrono::duration<float>;
                        auto constexpr repeat_delay = std::chrono::milliseconds(2); // 500 Hz rate

                        auto const motion_step_time = clock->now();
                        auto const t = std::chrono::duration_cast<SecondF>(motion_step_time - motion_start_time);

                        // duration_cast is not constexpr yet
                        float const dt = std::chrono::duration_cast<SecondF>(repeat_delay).count();

                        auto const state_copy = *state.lock();

                        // Normalize the speed so it's in
                        // pixels/alarm_invocation, and not pixels/second
                        auto const speed = state_copy.acceleration_curve.evaluate(t.count()) * dt;

                        motion_direction = {0, 0};
                        auto const buttons_down = state_copy.buttons_down;
                        if (buttons_down & directional_buttons_up)
                            motion_direction.dy += geom::DeltaYF{-speed};
                        if (buttons_down & directional_buttons_down)
                            motion_direction.dy += geom::DeltaYF{speed};
                        if (buttons_down & directional_buttons_left)
                            motion_direction.dx += geom::DeltaXF{-speed};
                        if (buttons_down & directional_buttons_right)
                            motion_direction.dx += geom::DeltaXF{speed};


                        // Handle two opposite buttons being pressed
                        if(buttons_down & directional_buttons_left && buttons_down & directional_buttons_right)
                            motion_direction.dx = geom::DeltaXF{0};

                        if (buttons_down & directional_buttons_up && buttons_down & directional_buttons_down)
                            motion_direction.dy = geom::DeltaYF{0};

                        auto const fabs = [](auto delta)
                        {
                            return decltype(delta){std::fabs(delta.as_value())};
                        };

                        auto const sign = [](auto delta)
                        {
                            return delta.as_value() > 0 ? 1 : -1;
                        };

                        auto const max_speed_ = state_copy.max_speed_;
                        if(max_speed_.dx.as_value() > 0)
                            motion_direction.dx =
                                sign(motion_direction.dx) * std::min(fabs(motion_direction.dx), max_speed_.dx * dt);

                        if(max_speed_.dy.as_value() > 0)
                            motion_direction.dy =
                                sign(motion_direction.dy) * std::min(fabs(motion_direction.dy), max_speed_.dy * dt);

                        // If the cursor stops moving without releasing
                        // all buttons (if for example you press two
                        // opposite directions), reset the time for
                        // acceleration.
                        if (motion_direction.length_squared() == 0)
                            motion_start_time = motion_step_time;

                        dispatcher(builder->pointer_event(
                            std::nullopt,
                            mir_pointer_action_motion,
                            state_copy.is_dragging ? state_copy.current_button : 0,
                            std::nullopt,
                            motion_direction,
                            mir_pointer_axis_source_none,
                            mir::events::ScrollAxisH{},
                            mir::events::ScrollAxisV{}));

                        if (auto const& repeat_alarm = weak_self->lock())
                            repeat_alarm->reschedule_in(repeat_delay);
                    });

                *shared_weak_alarm = motion_event_generator;
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

bool mir::shell::BasicMouseKeysTransformer::handle_click(
    MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_repeat || keyboard_action== mir_keyboard_action_down)
        return true;

    press_current_cursor_button(dispatcher, builder);
    release_current_cursor_button(dispatcher, builder);

    return true;
}

bool mir::shell::BasicMouseKeysTransformer::handle_change_pointer_button(
    MirKeyboardAction keyboard_action,
    input::MouseKeysKeymap::Action mousekeys_action,
    Dispatcher const& dispatcher,
    mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_down || keyboard_action == mir_keyboard_action_repeat)
        return true;

    {
        auto const locked_state = state.lock();
        auto& current_button = locked_state->current_button;
        // Up events
        switch (mousekeys_action)
        {
        case button_primary:
            current_button = mir_pointer_button_primary;
            break;
        case button_tertiary:
            current_button = mir_pointer_button_tertiary;
            break;
        case button_secondary:
            current_button = mir_pointer_button_secondary;
            break;
        default:
            std::unreachable();
        }
    }

    if (state.lock()->current_button)
        release_current_cursor_button(dispatcher, builder);

    return true;
}

bool mir::shell::BasicMouseKeysTransformer::handle_double_click(
    MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_down || keyboard_action == mir_keyboard_action_repeat)
        return true;

    for(auto i = 0; i < 2; i++)
    {
        press_current_cursor_button(dispatcher, builder);
        release_current_cursor_button(dispatcher, builder);
    }

    return true;
}

bool mir::shell::BasicMouseKeysTransformer::handle_drag(
    MirKeyboardAction keyboard_action,
    input::MouseKeysKeymap::Action mousekeys_action,
    Dispatcher const& dispatcher,
    mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_down || keyboard_action == mir_keyboard_action_repeat)
        return true;

    if (mousekeys_action == drag_start)
    {
        press_current_cursor_button(dispatcher, builder);
        state.lock()->is_dragging = true;
    }
    else
    {
        // Automatically releases
        release_current_cursor_button(dispatcher, builder);
    }

    return true;
}

mir::shell::BasicMouseKeysTransformer::AccelerationCurve::AccelerationCurve(
    double quadratic_factor, double linear_factor, double constant_factor) :
    quadratic_factor{quadratic_factor},
    linear_factor{linear_factor},
    constant_factor{constant_factor}
{
}

double mir::shell::BasicMouseKeysTransformer::AccelerationCurve::evaluate(double t) const
{
    return quadratic_factor * t * t + linear_factor * t + constant_factor;
}

void mir::shell::BasicMouseKeysTransformer::press_current_cursor_button(
    Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
{
    dispatcher(builder->pointer_event(
        std::nullopt,
        mir_pointer_action_button_down,
        state.lock()->current_button,
        std::nullopt,
        {0, 0},
        mir_pointer_axis_source_none,
        mir::events::ScrollAxisH{},
        mir::events::ScrollAxisV{}));
}

void mir::shell::BasicMouseKeysTransformer::release_current_cursor_button(
    Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
{
    dispatcher(builder->pointer_event(
        std::nullopt,
        mir_pointer_action_button_up,
        0,
        std::nullopt,
        {0, 0},
        mir_pointer_axis_source_none,
        mir::events::ScrollAxisH{},
        mir::events::ScrollAxisV{}));


    state.lock()->is_dragging = false;
}

void mir::shell::BasicMouseKeysTransformer::keymap(input::MouseKeysKeymap const& new_keymap)
{
    state.lock()->keymap_ = new_keymap;
}

void mir::shell::BasicMouseKeysTransformer::acceleration_factors(double constant, double linear, double quadratic)
{
    state.lock()->acceleration_curve = {quadratic, linear, constant};
}

void mir::shell::BasicMouseKeysTransformer::max_speed(double x_axis, double y_axis)
{
    auto const clamp_max_speed = [](auto configured_max_speed)
    {
        auto clamped_max_speed = configured_max_speed;
        if (configured_max_speed.dx < geometry::DeltaXF{0})
        {
            mir::log_warning("max speed x set to a value less than zero, clamping to zero");
            clamped_max_speed.dx = std::max(configured_max_speed.dx, geometry::DeltaXF{0.0});
        }

        if (configured_max_speed.dy < geometry::DeltaYF{0})
        {
            mir::log_warning("max speed y set to a value less than zero, clamping to zero");
            clamped_max_speed.dy = std::max(configured_max_speed.dy, geometry::DeltaYF{0.0});
        }

        return clamped_max_speed;
    };

    state.lock()->max_speed_ = clamp_max_speed(geometry::DisplacementF{x_axis, y_axis});
}

