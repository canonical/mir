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

#include "basic_simulated_secondary_click_transformer.h"

#include "mir/events/pointer_event.h"
#include "mir/main_loop.h"

#include <cassert>
#include <cmath>
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
    switch (mutable_state.lock()->state)
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
                            {
                                auto const state = mutable_state.lock();
                                state->on_secondary_click();

                                // Reset the state machine
                                state->state = State::waiting_for_real_left_down;
                            }
                            click(dispatcher, builder, mir_pointer_button_secondary);
                        });
                }

                consumed_left_down = std::unique_ptr<MirPointerEvent>(pointer_event->clone());
                secondary_click_dispatcher->reschedule_in(mutable_state.lock()->hold_duration);
                initial_position = pointer_event->position().value_or(initial_position);
                {
                    auto const state = mutable_state.lock();
                    state->state = State::waiting_for_motion_or_real_left_up;
                    state->on_hold_start();
                }

                return true; // Consume event
            }
        }
        break;
    case State::waiting_for_motion_or_real_left_up: // This state implies that the left button was pressed down
        {
            if (action == mir_pointer_action_motion)
            {
                auto const motion_threshold_squared = std::pow(mutable_state.lock()->displacement_threshold, 2);
                auto const displacement =
                    (pointer_event->position().value_or(initial_position) - initial_position).length_squared();
                if (displacement <= motion_threshold_squared)
                    return true;

                auto const state = mutable_state.lock();
                if (secondary_click_dispatcher->cancel())
                    state->on_hold_cancel();

                // Down event instead of the one we consumed the last time around
                consumed_left_down->set_event_time(pointer_event->event_time());
                dispatcher(std::move(consumed_left_down));

                // Re-dispatch the event we're currently handling in the proper
                // order. If we return `false`, the motion event we're handling
                // right now will be processed before the down event.
                //
                // It will be in the order: motion (you are here), "fake" down
                // event, "fake" motion event.
                dispatcher(std::make_shared<MirPointerEvent>(*pointer_event));

                state->state = State::waiting_for_drag_end_left_up;
                return true;
            }

            // `pointer_event->buttons()` contains the _current_ button state.
            // Since we're in this state machine state, it's implicitly known
            // that the left button was pressed, so we just need to make sure
            // it's not pressed now.
            if (action == mir_pointer_action_button_up && !(pointer_event->buttons() & mir_pointer_button_primary))
            {
                auto const state = mutable_state.lock();

                // If we get an up event BEFORE the alarm is triggered, that means
                // the user let go of the _left_ mouse button, we should cancel the
                // pending right click and issue a left click.
                if (secondary_click_dispatcher->cancel())
                {
                    state->on_hold_cancel();
                    click(dispatcher, builder, mir_pointer_button_primary);
                }

                state->state = State::waiting_for_real_left_down;
                return true;
            }
        }
        break;
    case State::waiting_for_drag_end_left_up:
        {
            // Let it through, just change state iff its an up event.
            // Motion events should stay in this state.
            if (action == mir_pointer_action_button_up)
                mutable_state.lock()->state = State::waiting_for_real_left_down;
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

void mir::shell::BasicSimulatedSecondaryClickTransformer::displacement_threshold(float displacement)
{
    mutable_state.lock()->displacement_threshold = displacement;
}

void mir::shell::BasicSimulatedSecondaryClickTransformer::enabled()
{
    mutable_state.lock()->on_enabled();
}

void mir::shell::BasicSimulatedSecondaryClickTransformer::disabled()
{
    mutable_state.lock()->on_disabled();
}

void mir::shell::BasicSimulatedSecondaryClickTransformer::enabled(std::function<void()>&& on_enabled)
{
    mutable_state.lock()->on_enabled = std::move(on_enabled);
}

void mir::shell::BasicSimulatedSecondaryClickTransformer::disabled(std::function<void()>&& on_disabled)
{
    mutable_state.lock()->on_disabled = std::move(on_disabled);
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
