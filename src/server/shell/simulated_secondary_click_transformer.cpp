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

#include "simulated_secondary_click_transformer.h"

#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/main_loop.h"

namespace
{
void click(
    mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
    mir::input::EventBuilder* builder,
    MirPointerButton button)
{
    dispatcher(builder->pointer_event(
        std::nullopt,
        mir_pointer_action_button_down,
        button,
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
}
} // namespace

mir::shell::SimulatedSecondaryClickTransformer::SimulatedSecondaryClickTransformer(
    std::shared_ptr<mir::MainLoop> const& main_loop) :
    main_loop{main_loop}
{
}

bool mir::shell::SimulatedSecondaryClickTransformer::transform_input_event(
    input::InputEventTransformer::EventDispatcher const& dispatcher,
    input::EventBuilder* builder,
    MirEvent const& event)
{
    if (event.type() != mir_event_type_input)
        return false;

    auto const input_event = event.to_input();
    if (input_event->input_type() != mir_input_event_type_pointer)
        return false;

    auto const pointer_event = input_event->to_pointer();
    auto const action = pointer_event->action();
    if (action != mir_pointer_action_button_down && action != mir_pointer_action_button_up)
        return false;

    switch (macro_state)
    {
    case MacroState::waiting_for_left_down:
        {
            if(action != mir_pointer_action_button_down)
                return false;

            if (pointer_event->buttons() & mir_pointer_button_primary)
            {
                if (!secondary_click_dispatcher)
                {
                    secondary_click_dispatcher = main_loop->create_alarm(
                        [dispatcher, builder]
                        {
                            click(dispatcher, builder, mir_pointer_button_secondary);
                        });
                }

                secondary_click_dispatcher->cancel();
                secondary_click_dispatcher->reschedule_in(std::chrono::seconds(1));

                macro_state = MacroState::synthesizing_left_or_right_click;
                micro_state = MicroState::waiting_for_synth_down;
                waiting_for_button = mir_pointer_button_secondary;

                return true;
            }
        }
        break;
    case MacroState::synthesizing_left_or_right_click:
        {
            if(action == mir_pointer_action_button_down && pointer_event->buttons() & waiting_for_button)
            {
                if (micro_state == MicroState::waiting_for_synth_down)
                {
                    micro_state = MicroState::waiting_for_synth_up;
                    return false;
                }
            }

            if(action == mir_pointer_action_button_up)
            {
                if (micro_state == MicroState::waiting_for_synth_up)
                {
                    if (waiting_for_button == mir_pointer_button_primary)
                    {
                        // If we're waiting for a synthesized left click up,
                        // then the user let go of the left button _before_ we
                        // synthesized the click. Since we're at the up half,
                        // the click is complete and the state machine should
                        // be retarted.
                        macro_state = MacroState::waiting_for_left_down;
                    }
                    else
                    {
                        // Leave the micro state machine and go to the next macro
                        // state. i.e. wait for the next _real_ up event, when the
                        // user lets go of the left mouse button
                        macro_state = MacroState::waiting_for_left_up;
                    }
                    return false;
                }

                // If we get an up event BEFORE the alarm is triggered, that
                // means the user let go of the _left_ mouse button, we should
                // cancel the pending right click, expect the down and up
                // events of that left click, and issue a left click.
                if (secondary_click_dispatcher->state() == time::Alarm::State::pending)
                {
                    secondary_click_dispatcher->cancel();

                    waiting_for_button = mir_pointer_button_primary;
                    micro_state = MicroState::waiting_for_synth_down;

                    click(dispatcher, builder, mir_pointer_button_primary);
                    return true;
                }
            }
        }
        break;
    case MacroState::waiting_for_left_up:
        {
            // Consume the up event generated by letting go of the left mouse
            // button and restart the macro machine
            if(action == mir_pointer_action_button_up)
            {
                macro_state = MacroState::waiting_for_left_down;
                return true;
            }
        }
        break;
    }

    return false;
}
