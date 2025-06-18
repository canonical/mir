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
#include "mir/input/cursor_observer.h"
#include "mir/input/cursor_observer_multiplexer.h"
#include "mir/input/event_builder.h"
#include "mir/main_loop.h"

namespace msh = mir::shell;

msh::BasicHoverClickTransformer::BasicHoverClickTransformer(
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<input::CursorObserverMultiplexer> const& cursor_observer_multiplexer) :
    main_loop{main_loop},
    cursor_observer{[&]
                    {
                        struct CursorObserver : public input::CursorObserver
                        {
                            CursorObserver(
                                mir::Synchronised<MutableState>& mutable_state,
                                std::unique_ptr<time::Alarm> hover_initializer) :
                                mutable_state{mutable_state},
                                hover_initializer{std::move(hover_initializer)}
                            {
                            }

                            void cursor_moved_to(float abs_x, float abs_y) override
                            {
                                auto const state = mutable_state.lock();
                                state->potential_position = {abs_x, abs_y};

                                // If a click already occured in the past. Only start a new hover click if
                                // the cursor moved "significantly" from the last position.
                                auto const hover_click_origin = state->hover_click_origin;
                                auto const cancel_displacement = state->cancel_displacement_threshold;

                                if (hover_click_origin)
                                {
                                    auto const distance_from_last_click =
                                        (*hover_click_origin - state->potential_position).length_squared();
                                    auto const reclick_threshold = reclick_displacement_threshold;

                                    // If we've moved too far, cancel the click.
                                    if (distance_from_last_click >= cancel_displacement * cancel_displacement &&
                                        state->click_dispatcher->state() == time::Alarm::State::pending)
                                    {
                                        state->click_dispatcher->cancel();
                                        state->on_hover_cancel();
                                        return;
                                    }

                                    // If we've moved too little, don't dispatch a new click
                                    if (distance_from_last_click <= (reclick_threshold * reclick_threshold))
                                        return;
                                }

                                // Cancel and reschedule to give users a grace period before the hover
                                // click actually starts. This also saves us from calling the
                                // start/cancel callbacks erroneously.
                                hover_initializer->cancel();
                                auto const grace_period = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    state->hover_duration * grace_period_percentage);
                                hover_initializer->reschedule_in(grace_period);
                            }

                            void pointer_usable() override
                            {
                            }

                            void pointer_unusable() override
                            {
                            }

                            mir::Synchronised<MutableState>& mutable_state;
                            std::unique_ptr<time::Alarm> const hover_initializer;
                        };

                        return std::make_shared<CursorObserver>(mutable_state, initialize_hover_initializer(main_loop));
                    }()}
{
    cursor_observer_multiplexer->register_interest(cursor_observer);
}

bool msh::BasicHoverClickTransformer::transform_input_event(
    mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
    mir::input::EventBuilder* builder,
    MirEvent const&)
{
    initialize_click_dispatcher(dispatcher, *builder);
    return false;
}

void mir::shell::BasicHoverClickTransformer::hover_duration(std::chrono::milliseconds delay)
{
    mutable_state.lock()->hover_duration = delay;
}

void mir::shell::BasicHoverClickTransformer::cancel_displacement_threshold(int displacement)
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

                auto const state = mutable_state.lock();
                state->on_click_dispatched();
                state->hover_click_origin.reset();
            });
    }
}

auto mir::shell::BasicHoverClickTransformer::initialize_hover_initializer(std::shared_ptr<mir::MainLoop> const& main_loop)
    -> std::unique_ptr<mir::time::Alarm>
{
    return main_loop->create_alarm(
        [&]
        {
            auto const state = mutable_state.lock();
            state->hover_click_origin = state->potential_position;
            auto const grace_period =
                std::chrono::duration_cast<std::chrono::milliseconds>(state->hover_duration * grace_period_percentage);

            state->click_dispatcher->reschedule_in(state->hover_duration - grace_period);
            state->on_hover_start();
        });
}

