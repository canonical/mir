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
#include "mir/events/keyboard_event.h"
#include "mir/events/pointer_event.h"
#include "mir/geometry/forward.h"
#include "mir/geometry/point.h"
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
            std::function<bool(MirInputEvent const*)> key_combination;

            std::chrono::milliseconds delay{500};
            mir::geometry::PointF cursor_position{0.0f, 0.0f}; // Assumes the cursor always starts at (0, 0)
        };

        LocatePointerFilter(
            std::shared_ptr<mir::MainLoop> const main_loop,
            std::shared_ptr<mir::Synchronised<State>> const initial_state) :
            state{initial_state},
            locate_pointer_alarm{main_loop->create_alarm(
                [this]
                {
                    auto const state = this->state->lock();
                    state->on_locate_pointer(state->cursor_position.x.as_value(), state->cursor_position.y.as_value());
                })}
        {
        }

        auto handle(MirEvent const& event) -> bool override
        {
            if (event.type() != mir_event_type_input)
                return false;

            auto const* input_event = event.to_input();
            switch (input_event->input_type())
            {
            case mir_input_event_type_key:
                handle_key_combination(input_event);
                break;
            case mir_input_event_type_pointer:
                record_pointer_position(input_event);
                break;
            default:
                break;
            }
            return false;
        }

        void handle_key_combination(MirInputEvent const* input_event)
        {
            if (!state->lock()->key_combination(input_event))
                return;

            auto const* keyboard_event = input_event->to_keyboard();
            switch (keyboard_event->action())
            {
            case mir_keyboard_action_down:
                locate_pointer_alarm->reschedule_in(state->lock()->delay);
                break;
            case mir_keyboard_action_up:
                locate_pointer_alarm->cancel();
                break;
            default:
                break;
            }
        }

        void record_pointer_position(MirInputEvent const* input_event)
        {
            auto const* pointer_event = input_event->to_pointer();
            if (auto position = pointer_event->position())
                state->lock()->cursor_position = *position;
        }

        std::shared_ptr<mir::Synchronised<State>> const state;
        std::unique_ptr<mir::time::Alarm> const locate_pointer_alarm;
    };

    Self(bool enable_by_default) :
        filter_state{std::make_shared<mir::Synchronised<LocatePointerFilter::State>>()}
    {
        state.lock()->enabled = enable_by_default;
        filter_state->lock()->key_combination = [](auto const* input_event)
        {
            auto const keyboard_event = mir_input_event_get_keyboard_event(input_event);
            if (!keyboard_event)
                return false;

            if (keyboard_event->keysym() != XKB_KEY_Control_R && keyboard_event->keysym() != XKB_KEY_Control_L)
                return false;
            return true;
        };
    }

    std::weak_ptr<mir::MainLoop> main_loop;
    std::weak_ptr<mir::input::CompositeEventFilter> composite_event_filter;

    struct State
    {
        bool enabled;
        std::shared_ptr<LocatePointerFilter> locate_pointer_filter;
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
            self->main_loop = server.the_main_loop();
            self->composite_event_filter = server.the_composite_event_filter();

            if (server.get_options()->get<bool>(enable_locate_pointer_opt))
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
        state->locate_pointer_filter = std::make_shared<Self::LocatePointerFilter>(main_loop, self->filter_state);
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

miral::LocatePointer& miral::LocatePointer::key_combination(std::function<bool(MirInputEvent const*)> key_combination)
{
    auto const state = self->state.lock();
    self->filter_state->lock()->key_combination = std::move(key_combination);

    return *this;
}
