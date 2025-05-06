/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/locate_pointer.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/geometry/forward.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/event_filter.h"
#include "mir/main_loop.h"
#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"

struct miral::LocatePointer::Self
{
    struct LocatePointerFilter : public mir::input::EventFilter
    {
        struct State
        {
            std::function<void(float, float)> on_locate_pointer{[](auto, auto) {}};
            std::function<void()> on_enabled{[] {}}, on_disabled{[] {}};

            std::chrono::milliseconds delay{500};
            mir::geometry::PointF cursor_position{0.0f, 0.0f}; // Assumes the cursor always starts at (0, 0)
        };

        LocatePointerFilter(std::shared_ptr<mir::Synchronised<State>> const initial_state) :
            state{initial_state}
        {
        }

        auto handle(MirEvent const& event) -> bool override
        {
            if (event.type() != mir_event_type_input)
                return false;

            auto const* input_event = event.to_input();
            if (input_event->input_type() != mir_input_event_type_pointer)
                return false;

            record_pointer_position(input_event);

            return false;
        }

        void record_pointer_position(MirInputEvent const* input_event)
        {
            auto const* pointer_event = input_event->to_pointer();
            if (auto position = pointer_event->position())
                state->lock()->cursor_position = *position;
        }

    private:
        std::shared_ptr<mir::Synchronised<State>> const state;
    };

    Self(bool enable_by_default) :
        filter_state{std::make_shared<mir::Synchronised<LocatePointerFilter::State>>()}
    {
        state.lock()->enabled = enable_by_default;
    }

    std::weak_ptr<mir::MainLoop> main_loop;
    std::weak_ptr<mir::input::CompositeEventFilter> composite_event_filter;

    struct State
    {
        bool enabled;
        std::shared_ptr<LocatePointerFilter> locate_pointer_filter;
        std::unique_ptr<mir::time::Alarm> locate_pointer_alarm;
    };

    std::shared_ptr<mir::Synchronised<LocatePointerFilter::State>> const filter_state;
    mir::Synchronised<State> state;
};

miral::LocatePointer::LocatePointer(bool enabled_by_default) :
    self(std::make_shared<Self>(enabled_by_default))
{
}

void miral::LocatePointer::operator()(mir::Server& server)
{
    constexpr auto* enable_locate_pointer_opt = "enable-locate-pointer";
    constexpr auto* locate_pointer_delay_opt = "locate-pointer-delay";

    {
        auto const state = self->state.lock();
        server.add_configuration_option(enable_locate_pointer_opt, "Enable locate pointer", state->enabled);
        server.add_configuration_option(
            locate_pointer_delay_opt,
            "Locate pointer delay in milliseconds",
            static_cast<int>(self->filter_state->lock()->delay.count()));
    }

    server.add_init_callback(
        [this, &server]
        {
            auto const main_loop = server.the_main_loop();
            self->main_loop = main_loop;
            self->composite_event_filter = server.the_composite_event_filter();

            self->state.lock()->locate_pointer_alarm = main_loop->create_alarm(
                [this]
                {
                    auto const state = this->self->filter_state->lock();
                    state->on_locate_pointer(state->cursor_position.x.as_value(), state->cursor_position.y.as_value());
                });

            auto const options = server.get_options();

            delay(std::chrono::milliseconds{options->get<int>(locate_pointer_delay_opt)});

            if (options->get<bool>(enable_locate_pointer_opt))
                enable();
            else
                disable();
        });
}

miral::LocatePointer& miral::LocatePointer::delay(std::chrono::milliseconds delay)
{
    self->filter_state->lock()->delay = delay;
    return *this;
}

miral::LocatePointer& miral::LocatePointer::on_locate_pointer(std::function<void(float x, float y)>&& on_locate_pointer)
{
    self->filter_state->lock()->on_locate_pointer = std::move(on_locate_pointer);
    return *this;
}

miral::LocatePointer& miral::LocatePointer::on_enabled(std::function<void()>&& on_enabled)
{
    self->filter_state->lock()->on_enabled = std::move(on_enabled);
    return *this;
}

miral::LocatePointer& miral::LocatePointer::on_disabled(std::function<void()>&& on_disabled)
{
    self->filter_state->lock()->on_disabled = std::move(on_disabled);
    return *this;
}

miral::LocatePointer& miral::LocatePointer::enable()
{
    auto const state = self->state.lock();
    state->enabled = true;

    // Already enabled
    if (state->locate_pointer_filter)
        return *this;

    if (auto const [composite_filter, main_loop] =
            std::pair{self->composite_event_filter.lock(), self->main_loop.lock()};
        composite_filter && main_loop)
    {
        state->locate_pointer_filter = std::make_shared<Self::LocatePointerFilter>(self->filter_state);
        // Need to account that this can be called from an event filter, which
        // when invoked has the composite filter's lock.
        main_loop->spawn(
            [composite_filter, self = this->self]
            {
                auto state = self->state.lock();
                composite_filter->append(state->locate_pointer_filter);
                self->filter_state->lock()->on_enabled();
            });
    }

    return *this;
}

miral::LocatePointer& miral::LocatePointer::disable()
{
    auto const state = self->state.lock();
    state->enabled = false;
    if (!state->locate_pointer_filter)
        return *this;

    state->locate_pointer_filter.reset();
    self->filter_state->lock()->on_disabled();

    return *this;
}

void miral::LocatePointer::schedule_request()
{
    auto const state = self->state.lock();
    state->locate_pointer_alarm->reschedule_in(self->filter_state->lock()->delay);
}

void miral::LocatePointer::cancel_request()
{
    auto const state = self->state.lock();
    state->locate_pointer_alarm->cancel();
}
