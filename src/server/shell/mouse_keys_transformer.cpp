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

#include "mir/input/input_sink.h"
#include "mir/main_loop.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>

mir::input::MouseKeysTransformer::MouseKeysTransformer(
    std::shared_ptr<mir::MainLoop> const& main_loop, std::shared_ptr<mir::options::Option> const& options) :
    main_loop{main_loop},
    motion_event_generator{std::make_shared<Synchronised<AlarmPtr>>()},
    acceleration_curve{options},
    max_speed{
        options->get<double>(mir::options::mouse_keys_max_speed_x),
        options->get<double>(mir::options::mouse_keys_max_speed_y)}
{
}

bool mir::input::MouseKeysTransformer::transform_input_event(
    std::shared_ptr<Dispatcher> const& dispatcher, MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;

    MirInputEvent const* input_event = mir_event_get_input_event(&event);

    if (mir_input_event_get_type(input_event) == mir_input_event_type_key)
    {
        MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);

        if (handle_motion(kev, dispatcher))
            return true;
        if (handle_click(kev, dispatcher))
            return true;
        if (handle_change_pointer_button(kev, dispatcher))
            return true;
        if (handle_double_click(kev, dispatcher))
            return true;
        if (handle_drag_start(kev, dispatcher))
            return true;
        if (handle_drag_end(kev, dispatcher))
            return true;
    }

    return false;
}

bool mir::input::MouseKeysTransformer::handle_motion(
    MirKeyboardEvent const* kev, std::shared_ptr<Dispatcher> const& dispatcher)
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
        {
            auto mev = motion_event_generator->lock();
            (*mev)->cancel();
            (*mev).reset();
        }

        return true;
    case mir_keyboard_action_down:
        {
            auto locked_motion_event_generator = motion_event_generator->lock();
            auto& mev = *locked_motion_event_generator;

            if (!mev)
            {
                mev = main_loop->create_alarm(
                    [&dispatcher,
                     this,
                     motion_start_time = std::chrono::steady_clock::now(),
                     locked_self = motion_event_generator] mutable
                    {
                        // Lock to prevent ourselves from being destroyed while executing
                        auto lock = locked_self->lock();

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
                        if (buttons_down & up)
                            motion_direction.dy += geom::DeltaYF{-speed};
                        if (buttons_down & down)
                            motion_direction.dy += geom::DeltaYF{speed};
                        if (buttons_down & left)
                            motion_direction.dx += geom::DeltaXF{-speed};
                        if (buttons_down & right)
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

                        dispatcher->dispatch_pointer_event(
                            std::nullopt,
                            mir_pointer_action_motion,
                            is_dragging ? current_button : 0,
                            std::nullopt,
                            motion_direction,
                            mir_pointer_axis_source_none,
                            mir::events::ScrollAxisH{},
                            mir::events::ScrollAxisV{});

                        (*lock)->reschedule_in(repeat_delay);
                    });

                mev->reschedule_in(std::chrono::milliseconds(0));
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
    MirKeyboardEvent const* kev, std::shared_ptr<Dispatcher> const& dispatcher)
{
    auto const action = mir_keyboard_event_action(kev);

    switch (mir_keyboard_event_keysym(kev))
    {
    case XKB_KEY_KP_5:
        switch (action)
        {
        case mir_keyboard_action_up:
            press_current_cursor_button(dispatcher);

            click_event_generator = main_loop->create_alarm(
                [dispatcher, this]
                {
                    release_current_cursor_button(dispatcher);
                });

            click_event_generator->reschedule_in(std::chrono::milliseconds(50));
            return true;
        case mir_keyboard_action_down:
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

bool mir::input::MouseKeysTransformer::handle_change_pointer_button(
    MirKeyboardEvent const* kev, std::shared_ptr<Dispatcher> const& dispatcher)
{
    auto const repeat_or_down = mir_keyboard_event_action(kev) == mir_keyboard_action_down ||
                                mir_keyboard_event_action(kev) == mir_keyboard_action_repeat;

    // Up events
    switch (mir_keyboard_event_keysym(kev))
    {
    case XKB_KEY_KP_Divide:
        if (!repeat_or_down)
            current_button = mir_pointer_button_primary;
        break;
    case XKB_KEY_KP_Multiply:
        if (!repeat_or_down)
            current_button = mir_pointer_button_tertiary;
        break;
    case XKB_KEY_KP_Subtract:
        if (!repeat_or_down)
            current_button = mir_pointer_button_secondary;
        break;
    default:
        return false;
    }

    if (current_button)
        release_current_cursor_button(dispatcher);

    return true;
}

bool mir::input::MouseKeysTransformer::handle_double_click(
    MirKeyboardEvent const* kev, std::shared_ptr<Dispatcher> const& dispatcher)
{
    if (mir_keyboard_event_keysym(kev) == XKB_KEY_KP_Add)
    {
        if (mir_keyboard_event_action(kev) == mir_keyboard_action_down ||
            mir_keyboard_event_action(kev) == mir_keyboard_action_repeat)
            return true;

        press_current_cursor_button(dispatcher);
        release_current_cursor_button(dispatcher);

        double_click_event_generator = main_loop->create_alarm(
            [dispatcher, this]
            {
                press_current_cursor_button(dispatcher);
                release_current_cursor_button(dispatcher);
            });

        double_click_event_generator->reschedule_in(std::chrono::milliseconds(100));

        return true;
    }

    return false;
}

bool mir::input::MouseKeysTransformer::handle_drag_start(
    MirKeyboardEvent const* kev, std::shared_ptr<Dispatcher> const& dispatcher)
{
    if (mir_keyboard_event_keysym(kev) == XKB_KEY_KP_0)
    {
        if (mir_keyboard_event_action(kev) == mir_keyboard_action_down ||
            mir_keyboard_event_action(kev) == mir_keyboard_action_repeat)
            return true;

        press_current_cursor_button(dispatcher);
        is_dragging = true;

        return true;
    }

    return false;
}

bool mir::input::MouseKeysTransformer::handle_drag_end(
    MirKeyboardEvent const* kev, std::shared_ptr<Dispatcher> const& dispatcher)
{
    if (mir_keyboard_event_keysym(kev) == XKB_KEY_KP_Decimal)
    {
        if (mir_keyboard_event_action(kev) == mir_keyboard_action_down ||
            mir_keyboard_event_action(kev) == mir_keyboard_action_repeat)
            return true;

        release_current_cursor_button(dispatcher);
        is_dragging = false;

        return true;
    }

    return false;
}

void mir::input::MouseKeysTransformer::press_current_cursor_button(std::shared_ptr<Dispatcher> const& dispatcher)
{
    dispatcher->dispatch_pointer_event(
        std::nullopt,
        mir_pointer_action_button_down,
        current_button,
        std::nullopt,
        {0, 0},
        mir_pointer_axis_source_none,
        mir::events::ScrollAxisH{},
        mir::events::ScrollAxisV{});
}

void mir::input::MouseKeysTransformer::release_current_cursor_button(std::shared_ptr<Dispatcher> const& dispatcher)
{
    dispatcher->dispatch_pointer_event(
        std::nullopt,
        mir_pointer_action_button_up,
        0,
        std::nullopt,
        {0, 0},
        mir_pointer_axis_source_none,
        mir::events::ScrollAxisH{},
        mir::events::ScrollAxisV{});

    is_dragging = false;
}

mir::input::MouseKeysTransformer::AccelerationCurve::AccelerationCurve(
    std::shared_ptr<mir::options::Option> const& options) :
    a{options->get<double>(mir::options::mouse_keys_acceleration_quadratic_factor)},
    b{options->get<double>(mir::options::mouse_keys_acceleration_linear_factor)},
    c{options->get<double>(mir::options::mouse_keys_acceleration_constant_factor)}
{
}

