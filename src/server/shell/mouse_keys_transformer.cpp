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

#include "mouse_keys_transformer.h"

#include "mir/geometry/forward.h"
#include "mir/log.h"
#include "mir/main_loop.h"
#include "mir/time/alarm.h"
#include "mir/input/mousekeys_common.h"

#include "mir/time/clock.h"
#include "mir_toolkit/events/enums.h"
#include "mir_toolkit/events/input/keyboard_event.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

using enum mir::input::MouseKeysKeymap::Action;

mir::input::MouseKeysKeymap const mir::input::MouseKeysTransformer::default_keymap = {
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

mir::input::MouseKeysTransformer::MouseKeysTransformer(
    std::shared_ptr<mir::MainLoop> const& main_loop,
    geometry::DisplacementF configured_max_speed,
    AccelerationParameters const& params,
    std::shared_ptr<time::Clock> const& clock) :
    MouseKeysTransformer(main_loop, configured_max_speed, params, clock, MouseKeysTransformer::default_keymap)
{
}

mir::input::MouseKeysTransformer::MouseKeysTransformer(
    std::shared_ptr<mir::MainLoop> const& main_loop,
    geometry::DisplacementF configured_max_speed,
    AccelerationParameters const& params,
    std::shared_ptr<time::Clock> const& clock,
    MouseKeysKeymap keymap) :
    main_loop{main_loop},
    clock{clock},
    acceleration_curve{params},
    max_speed{[configured_max_speed]
              {
                  auto clamped_max_speed = configured_max_speed;
                  if (configured_max_speed.dx < geometry::DeltaXF{0})
                  {
                      mir::log_warning("max speed x set to a value less than zero, clamping to zero");
                      clamped_max_speed.dx = std::max(configured_max_speed.dx, geometry::DeltaXF{0.0});
                  }

                  if (configured_max_speed.dy < geometry::DeltaYF{0})
                  {
                      mir::log_warning(
                          "max speed y set to a value less than zero, clamping to zero");
                      clamped_max_speed.dy = std::max(configured_max_speed.dy, geometry::DeltaYF{0.0});
                  }

                  return clamped_max_speed;
              }()},
    keymap{keymap}
{
}

bool mir::input::MouseKeysTransformer::transform_input_event(
    mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
    mir::input::EventBuilder* builder,
    MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;

    MirInputEvent const* input_event = mir_event_get_input_event(&event);

    if (mir_input_event_get_type(input_event) == mir_input_event_type_key)
    {
        std::lock_guard guard{state_mutex};

        MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
        auto const keysym = mir_keyboard_event_keysym(kev);
        auto const keyboard_action = mir_keyboard_event_action(kev);

        if (auto const mousekey_action = keymap.get_action(keysym))
        {
            switch (*mousekey_action)
            {
            case move_left:
            case move_right:
            case move_up:
            case move_down:
                return handle_motion(keyboard_action, *mousekey_action, dispatcher, builder);
            case click:
                return (handle_click(keyboard_action, dispatcher, builder));
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

void mir::input::MouseKeysTransformer::set_keymap(MouseKeysKeymap const& new_keymap)
{
    std::lock_guard guard{state_mutex};
    keymap = new_keymap;
}

bool mir::input::MouseKeysTransformer::handle_motion(
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

    switch (keyboard_action)
    {
    case mir_keyboard_action_up:
        if (buttons_down == directional_buttons_none)
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
                     weak_self = shared_weak_alarm] mutable
                    {
                        std::lock_guard guard{state_mutex};

                        using SecondF = std::chrono::duration<float>;
                        auto constexpr repeat_delay = std::chrono::milliseconds(2); // 500 Hz rate

                        auto const motion_step_time = clock->now();
                        auto const t = std::chrono::duration_cast<SecondF>(motion_step_time - motion_start_time);

                        // duration_cast is not constexpr yet
                        float const dt = std::chrono::duration_cast<SecondF>(repeat_delay).count();

                        // Normalize the speed so it's in
                        // pixels/alarm_invocation, and not pixels/second
                        auto const speed = acceleration_curve.evaluate(t.count()) * dt;

                        motion_direction = {0, 0};
                        if (buttons_down & directional_buttons_up)
                            motion_direction.dy += geom::DeltaYF{-speed};
                        if (buttons_down & directional_buttons_down)
                            motion_direction.dy += geom::DeltaYF{speed};
                        if (buttons_down & directional_buttons_left)
                            motion_direction.dx += geom::DeltaXF{-speed};
                        if (buttons_down & directional_buttons_right)
                            motion_direction.dx += geom::DeltaXF{speed};

                        auto const fabs = [](auto delta)
                        {
                            return decltype(delta){std::fabs(delta.as_value())};
                        };

                        auto const sign = [](auto delta)
                        {
                            return delta.as_value() > 0 ? 1 : -1;
                        };

                        if(max_speed.dx.as_value() > 0)
                            motion_direction.dx =
                                sign(motion_direction.dx) * std::min(fabs(motion_direction.dx), max_speed.dx * dt);

                        if(max_speed.dy.as_value() > 0)
                            motion_direction.dy =
                                sign(motion_direction.dy) * std::min(fabs(motion_direction.dy), max_speed.dy * dt);

                        // If the cursor stops moving without releasing
                        // all buttons (if for example you press two
                        // opposite directions), reset the time for
                        // acceleration.
                        if (motion_direction.length_squared() == 0)
                            motion_start_time = motion_step_time;

                        dispatcher(builder->pointer_event(
                            std::nullopt,
                            mir_pointer_action_motion,
                            is_dragging ? current_button : 0,
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

bool mir::input::MouseKeysTransformer::handle_click(
    MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_repeat || keyboard_action== mir_keyboard_action_down)
        return true;

    press_current_cursor_button(dispatcher, builder);

    click_event_generator = main_loop->create_alarm(
        [dispatcher, this, builder]
        {
            release_current_cursor_button(dispatcher, builder);
        });

    click_event_generator->reschedule_in(std::chrono::milliseconds(50));
    return true;
}

bool mir::input::MouseKeysTransformer::handle_change_pointer_button(
    MirKeyboardAction keyboard_action,
    MouseKeysKeymap::Action mousekeys_action,
    Dispatcher const& dispatcher,
    mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_down || keyboard_action == mir_keyboard_action_repeat)
        return true;

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

    if (current_button)
        release_current_cursor_button(dispatcher, builder);

    return true;
}

bool mir::input::MouseKeysTransformer::handle_double_click(
    MirKeyboardAction keyboard_action, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_down || keyboard_action == mir_keyboard_action_repeat)
        return true;

    press_current_cursor_button(dispatcher, builder);
    release_current_cursor_button(dispatcher, builder);

    double_click_event_generator = main_loop->create_alarm(
        [dispatcher, this, builder]
        {
            press_current_cursor_button(dispatcher, builder);
            release_current_cursor_button(dispatcher, builder);
        });

    double_click_event_generator->reschedule_in(std::chrono::milliseconds(100));

    return true;
}

bool mir::input::MouseKeysTransformer::handle_drag(
    MirKeyboardAction keyboard_action,
    MouseKeysKeymap::Action mousekeys_action,
    Dispatcher const& dispatcher,
    mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_down || keyboard_action == mir_keyboard_action_repeat)
        return true;

    if (mousekeys_action == drag_start)
    {
        press_current_cursor_button(dispatcher, builder);
        is_dragging = true;
    }
    else
    {
        release_current_cursor_button(dispatcher, builder);
        is_dragging = false;
    }

    return true;
}

mir::input::MouseKeysTransformer::AccelerationCurve::AccelerationCurve(AccelerationParameters const& params) :
    params(params)
{
}

double mir::input::MouseKeysTransformer::AccelerationCurve::evaluate(double t) const
{
    return params.a * t * t + params.b * t + params.c;
}

void mir::input::MouseKeysTransformer::press_current_cursor_button(
    Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
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
}

void mir::input::MouseKeysTransformer::release_current_cursor_button(
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

    is_dragging = false;
}

void mir::input::MouseKeysTransformer::set_acceleration_factors(double constant, double linear, double quadratic)
{
    std::lock_guard guard{state_mutex};
    acceleration_curve.params = {quadratic, linear, constant};
}

void mir::input::MouseKeysTransformer::set_max_speed(double x_axis, double y_axis)
{
    std::lock_guard guard{state_mutex};
    max_speed = geometry::DisplacementF{x_axis, y_axis};
}

