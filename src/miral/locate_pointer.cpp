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
#include "mir/log.h"
#include "mir/main_loop.h"
#include "mir/server.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"
#include "miral/live_config.h"

namespace geom = mir::geometry;

struct miral::LocatePointer::Self
{
    struct PointerPositionRecorder : public mir::input::EventFilter
    {
        struct State
        {
            mir::geometry::PointF cursor_position{0.0f, 0.0f}; // Assumes the cursor always starts at (0, 0)
        };

        auto handle(MirEvent const& event) -> bool override
        {
            if (event.type() != mir_event_type_input)
                return false;

            auto const* input_event = event.to_input();
            if (input_event->input_type() != mir_input_event_type_pointer)
                return false;

            auto const* pointer_event = input_event->to_pointer();

            if (pointer_event->action() != mir_pointer_action_motion)
                return false;
            if (auto position = pointer_event->position())
                state.lock()->cursor_position = *position;

            return false;
        }

        mir::Synchronised<State> state;
    };

    Self(bool enable_by_default) :
        state{State{enable_by_default, std::make_shared<Self::PointerPositionRecorder>()}}
    {
    }

    void on_server_init(mir::Server& server)
    {
        auto const state_ = state.lock();
        state_->locate_pointer_alarm = server.the_main_loop()->create_alarm(
            [this]
            {
                auto const self_state = state.lock();
                auto const filter_state = self_state->pointer_position_recorder->state.lock();

                self_state->on_locate_pointer(filter_state->cursor_position);
            });

        server.the_composite_event_filter()->append(state_->pointer_position_recorder);
    }

    struct State
    {
        State(bool enabled, std::shared_ptr<PointerPositionRecorder> const& position_recorder) :
            enabled{enabled},
            pointer_position_recorder{position_recorder}
        {
        }

        bool enabled;
        std::shared_ptr<PointerPositionRecorder> const pointer_position_recorder;

        std::unique_ptr<mir::time::Alarm> locate_pointer_alarm;
        std::function<void(geom::PointF)> on_locate_pointer{[](auto) {}};
        std::chrono::milliseconds delay{500};
    };

    mir::Synchronised<State> state;
};

miral::LocatePointer::LocatePointer(std::shared_ptr<Self> self)
    : self{std::move(self)}
{
}

auto miral::LocatePointer::enabled() -> LocatePointer
{
    return LocatePointer{std::make_shared<Self>(true)};
}

auto miral::LocatePointer::disabled() -> LocatePointer
{
    return LocatePointer{std::make_shared<Self>(false)};
}

void miral::LocatePointer::operator()(mir::Server& server)
{
    server.add_init_callback(
        [this, &server]
        {
            self->on_server_init(server);
        });
}

miral::LocatePointer::LocatePointer(miral::live_config::Store& config_store) :
    self{std::make_shared<Self>(false)}
{
    auto const s = self->state.lock();

    config_store.add_bool_attribute(
        {"locate_pointer", "enable"},
        "Enable or disable slow keys",
        s->enabled,
        [this](live_config::Key const&, std::optional<bool> val)
        {
            if (val)
            {
                if (*val)
                    this->enable();
                else
                    this->disable();
            }
        });

    config_store.add_int_attribute(
        {"locate_pointer", "delay"},
        "The delay between scheduling a request and the on_locate_pointer callback being called.",
        s->delay.count(),
        [this](live_config::Key const& key, std::optional<int> val)
        {
            if (val)
            {
                if (*val >= 0)
                {
                    this->delay(std::chrono::milliseconds{*val});
                }
                else
                {
                    mir::log_warning(
                        "Config value %s does not support negative values. Ignoring the supplied value (%d)...",
                        key.to_string().c_str(),
                        *val);
                }
            }
        });
}

miral::LocatePointer& miral::LocatePointer::delay(std::chrono::milliseconds delay)
{
    self->state.lock()->delay = delay;
    return *this;
}

miral::LocatePointer& miral::LocatePointer::on_locate_pointer(std::function<void(geom::PointF)>&& on_locate_pointer)
{
    self->state.lock()->on_locate_pointer = std::move(on_locate_pointer);
    return *this;
}

miral::LocatePointer& miral::LocatePointer::enable()
{
    auto const state = self->state.lock();

    if (state->enabled)
        return *this;

    state->enabled = true;

    return *this;
}

miral::LocatePointer& miral::LocatePointer::disable()
{
    auto const state = self->state.lock();

    if (!state->enabled)
        return *this;

    state->enabled = false;
    state->locate_pointer_alarm->cancel();

    return *this;
}

void miral::LocatePointer::schedule_request()
{
    auto const state = self->state.lock();
    if(!state->enabled)
        return;

    state->locate_pointer_alarm->reschedule_in(state->delay);
}

void miral::LocatePointer::cancel_request()
{
    auto const state = self->state.lock();
    state->locate_pointer_alarm->cancel();
}
