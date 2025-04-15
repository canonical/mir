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

#include "locate_pointer_filter.h"

#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/events/pointer_event.h"
#include "mir/main_loop.h"

mir::shell::LocatePointerFilter::LocatePointerFilter(
    std::shared_ptr<mir::MainLoop> const main_loop, State& initial_state) :
    state{initial_state},
    main_loop{main_loop},
    locate_pointer_alarm{main_loop->create_alarm(
        [this]
        {
            state.on_locate_pointer(state.cursor_position.x.as_value(), state.cursor_position.y.as_value());
        })}
{
}

auto mir::shell::LocatePointerFilter::handle(MirEvent const& event) -> bool
{
    if(event.type() != mir_event_type_input)
        return false;

    auto const* input_event = event.to_input();
    switch (input_event->input_type())
    {
    case mir_input_event_type_key:
        return handle_ctrl(input_event);
    case mir_input_event_type_pointer:
        record_pointer_position(input_event);
        return false;
    default:
        return false;
    }
}

auto mir::shell::LocatePointerFilter::handle_ctrl(MirInputEvent const* input_event) -> bool
{
    auto const* keyboard_event = input_event->to_keyboard();
    if (keyboard_event->keysym() != XKB_KEY_Control_R && keyboard_event->keysym() != XKB_KEY_Control_L)
        return false;

    switch (keyboard_event->action())
    {
    case mir_keyboard_action_down:
        locate_pointer_alarm->reschedule_in(state.delay);
        break;
    case mir_keyboard_action_up:
        locate_pointer_alarm->cancel();
        break;
    default:
        break;
    }

    return false;
}

void mir::shell::LocatePointerFilter::record_pointer_position(MirInputEvent const* input_event)
{
    auto const* pointer_event = input_event->to_pointer();
    if (auto position = pointer_event->position())
        state.cursor_position = *position;
}
