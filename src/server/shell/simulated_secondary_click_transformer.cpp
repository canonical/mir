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
#include "mir/events/event_builders.h"

#include <cassert>
#include <mir/log.h>
#include <mir/event_printer.h>

#include <utility>

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

mir::shell::BasicSimulatedSecondaryClickTransformer::BasicSimulatedSecondaryClickTransformer(
    std::shared_ptr<mir::MainLoop> const& main_loop, MirInputDeviceId virtual_device_id) :
    main_loop{main_loop},
    virtual_device_id{virtual_device_id}
{
}

bool mir::shell::BasicSimulatedSecondaryClickTransformer::transform_input_event(
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
    // Skip events dispatched from transformers using the virtual device
    if(pointer_event->device_id() == virtual_device_id)
        return false;

    auto const action = pointer_event->action();
    switch (state)
    {
    case State::waiting_for_real_left_down:
        {
            if (action == mir_pointer_action_button_down && (pointer_event->buttons() & mir_pointer_button_primary))
            {
                if (!secondary_click_dispatcher)
                {
                    secondary_click_dispatcher = main_loop->create_alarm(
                        [dispatcher, builder, this]
                        {
                            mutable_state.lock()->on_secondary_click();
                            click(dispatcher, builder, mir_pointer_button_secondary);
                        });
                }

                consumed_left_down = std::unique_ptr<MirPointerEvent>(pointer_event->clone());
                secondary_click_dispatcher->reschedule_in(mutable_state.lock()->hold_duration);
                state = State::waiting_for_motion_or_real_left_up;
                mutable_state.lock()->on_hold_start();

                return true; // Consume event
            }
        }
        break;
    case State::waiting_for_motion_or_real_left_up: // This state implies that the left button was pressed down
        {
            if (action == mir_pointer_action_motion)
            {
                if (secondary_click_dispatcher->state() == time::Alarm::pending)
                {
                    secondary_click_dispatcher->cancel();
                    mutable_state.lock()->on_hold_cancel();
                }

                // Down event instead of the one we consumed the last time around
                consumed_left_down->set_event_time(pointer_event->event_time());
                dispatcher(std::move(consumed_left_down));

                // Re-dispatch the event we're currently handling in the proper
                // order. If we return `false`, the motion event we're handling
                // right now will be processed before the down event.
                //
                // It will be in the order: motion (you are here), "fake" down
                // event, "fake" motion event.
                dispatcher(
                    mir::events::make_pointer_event(
                        pointer_event->device_id(),
                        pointer_event->event_time(),
                        pointer_event->modifiers(),
                        pointer_event->action(),
                        pointer_event->buttons(),
                        pointer_event->position(),
                        pointer_event->motion(),
                        pointer_event->axis_source(),
                        pointer_event->h_scroll(),
                        pointer_event->v_scroll()));

                state = State::waiting_for_drag_end_left_up;
                return true;
            }

            // `pointer_event->buttons()` contains the _current_ button state.
            // Since we're in this state machine state, it's implicitly known
            // that the left button was pressed, so we just need to make sure
            // it's not pressed now.
            if (action == mir_pointer_action_button_up && !(pointer_event->buttons() & mir_pointer_button_primary))
            {
                // If we get an up event BEFORE the alarm is triggered, that means
                // the user let go of the _left_ mouse button, we should cancel the
                // pending right click and issue a left click.
                if (secondary_click_dispatcher->state() == time::Alarm::State::pending)
                {
                    mutable_state.lock()->on_hold_cancel();
                    click(dispatcher, builder, mir_pointer_button_primary);
                }

                // Whether the user cancelled early or not, reset the state
                // machine and the alarm, and consume the event.
                state = State::waiting_for_real_left_down;
                secondary_click_dispatcher->cancel();
                return false;
            }
        }
        break;
    case State::waiting_for_drag_end_left_up:
        {
            // Let it through, just change state iff its an up event.
            // Motion events should stay in this state.
            if (action == mir_pointer_action_button_up)
                state = State::waiting_for_real_left_down;
        }
        break;
    }

    // Pass through all other events
    return false;
}

void mir::shell::BasicSimulatedSecondaryClickTransformer::hold_duration(std::chrono::milliseconds delay)
{
    mutable_state.lock()->hold_duration = delay;
}

void mir::shell::BasicSimulatedSecondaryClickTransformer::hold_start(std::function<void()>&& on_hold_start)
{
    mutable_state.lock()->on_hold_start = std::move(on_hold_start);
}

void mir::shell::BasicSimulatedSecondaryClickTransformer::hold_cancel(std::function<void()>&& on_hold_cancel)
{
    mutable_state.lock()->on_hold_cancel = std::move(on_hold_cancel);
}

void mir::shell::BasicSimulatedSecondaryClickTransformer::secondary_click(std::function<void()>&& on_secondary_click)
{
    mutable_state.lock()->on_secondary_click = std::move(on_secondary_click);
}
