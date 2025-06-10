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

#include "basic_hover_click_transformer.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/input/event_builder.h"
#include "mir/main_loop.h"

namespace msh = mir::shell;

msh::BasicHoverClickTransformer::BasicHoverClickTransformer(std::shared_ptr<MainLoop> const& main_loop) :
    main_loop{main_loop},
    hover_initializer{[&main_loop, this]
                      {
                          return main_loop->create_alarm(
                              [this]
                              {
                                  auto const state = mutable_state.lock();
                                  state->hover_click_origin = state->potential_position;
                                  auto const grace_period = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      state->hover_duration * grace_period_percentage);

                                  state->click_dispatcher->reschedule_in(state->hover_duration - grace_period);
                                  state->on_hover_start();
                              });
                      }()}
{
}

bool msh::BasicHoverClickTransformer::transform_input_event(
    mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
    mir::input::EventBuilder* builder,
    MirEvent const& event)
{
    auto const* input_event = event.to_input();
    if (!input_event)
        return false;

    auto const* pointer_event = input_event->to_pointer();
    if(!pointer_event)
        return false;

    initialize_click_dispatcher(dispatcher, *builder);

    // Hover-clicking only begins after the pointer is moved
    if(pointer_event->action() == mir_pointer_action_motion)
    {
        // If a click already occured in the past. Only start a new hover click if
        // the cursor moved "significantly" from the last position.
        auto const hover_click_origin = mutable_state.lock()->hover_click_origin;
        if(hover_click_origin && pointer_event->position())
        {
            auto const distance_from_last_click = (*hover_click_origin - *pointer_event->position()).length_squared();
            auto const threshold = reclick_displacement_threshold;
            if(distance_from_last_click <= (threshold * threshold))
                return false;
        }

        {
            auto const state = mutable_state.lock();

            // Cancel and reschedule to give users a grace period before the hover
            // click actually starts. This also saves us from calling the
            // start/cancel callbacks erroneously.
            hover_initializer->cancel();
            if (auto const position = pointer_event->position())
                state->potential_position = *position;
            auto const grace_period =
                std::chrono::duration_cast<std::chrono::milliseconds>(state->hover_duration * grace_period_percentage);
            hover_initializer->reschedule_in(grace_period);
            
            // Compute the distance from where the hover click "started", cancel if
            // we moved too far away.
            auto const origin = hover_click_origin.value_or(state->potential_position);
            auto const distance_squared = (pointer_event->position().value_or(origin) - origin).length_squared();
            auto const threshold = state->cancel_displacement_threshold;
            if (distance_squared > threshold * threshold)
            {
                // Possibly can have the click dispatcher execute between our check and
                // cancelling
                if (state->click_dispatcher->state() == time::Alarm::State::pending)
                {
                    state->click_dispatcher->cancel();
                    state->on_hover_cancel();
                }
            }
        }
    }

    return false;
}

void mir::shell::BasicHoverClickTransformer::hover_duration(std::chrono::milliseconds delay)
{
    mutable_state.lock()->hover_duration = delay;
}

void mir::shell::BasicHoverClickTransformer::cancel_displacement_threshold(float displacement)
{
    mutable_state.lock()->cancel_displacement_threshold = displacement;
}

void mir::shell::BasicHoverClickTransformer::on_hover_start(std::function<void()>&& on_hover_start)
{
    mutable_state.lock()->on_hover_start = std::move(on_hover_start);
}

void mir::shell::BasicHoverClickTransformer::on_hover_cancel(std::function<void()>&& on_hover_cancelled)
{
    mutable_state.lock()->on_hover_cancel = std::move(on_hover_cancelled);
}

void mir::shell::BasicHoverClickTransformer::on_click_dispatched(std::function<void()>&& on_click_dispatched)
{
    mutable_state.lock()->on_click_dispatched = std::move(on_click_dispatched);
}

void mir::shell::BasicHoverClickTransformer::initialize_click_dispatcher(
    mir::input::InputEventTransformer::EventDispatcher const& dispatcher, mir::input::EventBuilder& builder)
{
    auto state = mutable_state.lock();
    if (!state->click_dispatcher)
    {
        state->click_dispatcher = main_loop->create_alarm(
            [dispatcher, &builder, this]
            {
                auto down = builder.pointer_event(
                    std::nullopt,
                    mir_pointer_action_button_down,
                    mir_pointer_button_primary,
                    std::nullopt,
                    {0, 0},
                    mir_pointer_axis_source_none,
                    mir::events::ScrollAxisH{},
                    mir::events::ScrollAxisV{});

                auto up = builder.pointer_event(
                    std::nullopt,
                    mir_pointer_action_button_up,
                    0,
                    std::nullopt,
                    {0, 0},
                    mir_pointer_axis_source_none,
                    mir::events::ScrollAxisH{},
                    mir::events::ScrollAxisV{});

                dispatcher(std::move(down));
                dispatcher(std::move(up));

                mutable_state.lock()->on_click_dispatched();
            });
    }
}

