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
#include "mir/options/configuration.h"
#include "mir/time/alarm.h"
#include "mir/input/mousekeys_common.h"

#include "mir_toolkit/events/enums.h"
#include "mir_toolkit/events/input/keyboard_event.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

mir::input::MouseKeysTransformer::MouseKeysTransformer(
    std::shared_ptr<mir::MainLoop> const& main_loop,
    geometry::Displacement configured_max_speed,
    AccelerationParameters const& params) :
    main_loop{main_loop},
    acceleration_curve{params},
    max_speed{[configured_max_speed]
              {
                  auto clamped_max_speed = configured_max_speed;
                  if (configured_max_speed.dx < geometry::DeltaX{0})
                  {
                      mir::log_warning(
                          "%s set to a value less than zero, clamping to zero", mir::options::mouse_keys_max_speed_x);
                      clamped_max_speed.dx = std::max(configured_max_speed.dx, geometry::DeltaX{0.0});
                  }

                  if (configured_max_speed.dy < geometry::DeltaY{0})
                  {
                      mir::log_warning(
                          "%s set to a value less than zero, clamping to zero", mir::options::mouse_keys_max_speed_y);
                      clamped_max_speed.dy = std::max(configured_max_speed.dy, geometry::DeltaY{0.0});
                  }

                  return clamped_max_speed;
              }()},
    keymap{std::make_shared<MouseKeysKeymap>(MouseKeysKeymap{
        {XKB_KEY_KP_2, MouseKeysAction::move_down},
        {XKB_KEY_KP_4, MouseKeysAction::move_left},
        {XKB_KEY_KP_6, MouseKeysAction::move_right},
        {XKB_KEY_KP_8, MouseKeysAction::move_up},
        {XKB_KEY_KP_5, MouseKeysAction::click},
        {XKB_KEY_KP_Add, MouseKeysAction::double_click},
        {XKB_KEY_KP_0, MouseKeysAction::drag_start},
        {XKB_KEY_KP_Decimal, MouseKeysAction::drag_end},
        {XKB_KEY_KP_Divide, MouseKeysAction::button_primary},
        {XKB_KEY_KP_Multiply, MouseKeysAction::button_tertiary},
        {XKB_KEY_KP_Subtract, MouseKeysAction::button_secondary}})}
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

        if (!keymap->contains(keysym))
            return false;

        auto mousekey_action = keymap->at(keysym);
        switch (mousekey_action)
        {
        case MouseKeysAction::move_left:
        case MouseKeysAction::move_right:
        case MouseKeysAction::move_up:
        case MouseKeysAction::move_down:
            return handle_motion(keyboard_action, mousekey_action, dispatcher, builder);
        case MouseKeysAction::click:
            return (handle_click(keyboard_action, dispatcher, builder));
        case MouseKeysAction::double_click:
            return handle_double_click(keyboard_action, dispatcher, builder);
        case MouseKeysAction::drag_start:
        case MouseKeysAction::drag_end:
            return handle_drag(keyboard_action, mousekey_action, dispatcher, builder);
        case MouseKeysAction::button_primary:
        case MouseKeysAction::button_secondary:
        case MouseKeysAction::button_tertiary:
            return handle_change_pointer_button(keyboard_action, mousekey_action, dispatcher, builder);
        }
    }

    return false;
}

void mir::input::MouseKeysTransformer::set_keymap(MouseKeysKeymap const& new_keymap)
{
    std::lock_guard guard{state_mutex};
    keymap = std::make_unique<MouseKeysKeymap>(new_keymap);
}

bool mir::input::MouseKeysTransformer::handle_motion(
    MirKeyboardAction keyboard_action,
    MouseKeysAction mousekey_action,
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
    case MouseKeysAction::move_down:
        set_or_clear(buttons_down, directional_buttons_down, keyboard_action);
        break;
    case MouseKeysAction::move_left:
        set_or_clear(buttons_down, directional_buttons_left, keyboard_action);
        break;
    case MouseKeysAction::move_right:
        set_or_clear(buttons_down, directional_buttons_right, keyboard_action);
        break;
    case MouseKeysAction::move_up:
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
                     motion_start_time = std::chrono::steady_clock::now(),
                     weak_self = shared_weak_alarm] mutable
                    {
                        using SecondF = std::chrono::duration<float>;
                        auto constexpr repeat_delay = std::chrono::milliseconds(2); // 500 Hz rate

                        auto const motion_step_time = std::chrono::steady_clock::now();
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
    MouseKeysAction mousekeys_action,
    Dispatcher const& dispatcher,
    mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_down || keyboard_action == mir_keyboard_action_repeat)
        return true;

    // Up events
    switch (mousekeys_action)
    {
    case MouseKeysAction::button_primary:
        current_button = mir_pointer_button_primary;
        break;
    case MouseKeysAction::button_tertiary:
        current_button = mir_pointer_button_tertiary;
        break;
    case MouseKeysAction::button_secondary:
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
    MouseKeysAction mousekeys_action,
    Dispatcher const& dispatcher,
    mir::input::EventBuilder* const builder)
{
    if (keyboard_action == mir_keyboard_action_down || keyboard_action == mir_keyboard_action_repeat)
        return true;

    if (mousekeys_action == MouseKeysAction::drag_start)
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
