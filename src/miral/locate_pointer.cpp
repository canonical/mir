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
#include "mir/compositor/stream.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/events/pointer_event.h"
#include "mir/geometry/forward.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/event_filter.h"
#include "mir/main_loop.h"
#include "mir/options/option.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/scene/basic_surface.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_observer.h"
#include "mir/server.h"
#include "mir/shell/surface_stack.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"
#include <iostream>
#include <list>
#include <thread>

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

                self_state->on_locate_pointer(
                    filter_state->cursor_position.x.as_value(), filter_state->cursor_position.y.as_value());
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
        std::function<void(float, float)> on_locate_pointer{[](auto, auto) {}};
        std::function<void()> on_enabled{[] {}}, on_disabled{[] {}};
        std::chrono::milliseconds delay{500};
    };

    mir::Synchronised<State> state;
    std::shared_ptr<mir::compositor::Stream> stream;
    std::shared_ptr<mir::graphics::Buffer> buffer;
    std::shared_ptr<mir::scene::BasicSurface> surface;
    std::shared_ptr<mir::scene::SurfaceObserver> observer;
};

miral::LocatePointer::LocatePointer(bool enabled_by_default) :
    self(std::make_shared<Self>(enabled_by_default))
{
}

namespace
{
auto foo(mir::Server& server) -> std::tuple<
    std::shared_ptr<mir::compositor::Stream>,
    std::shared_ptr<mir::graphics::Buffer>,
    std::shared_ptr<mir::scene::BasicSurface>,
    std::shared_ptr<mir::scene::SurfaceObserver>>
{
    auto buffer =
        server.the_buffer_allocator()->alloc_software_buffer(mir::geometry::Size(200, 200), mir_pixel_format_abgr_8888);

    auto w = std::dynamic_pointer_cast<mir::renderer::software::RWMappableBuffer>(buffer)->map_writeable();
    for (size_t i = 0; i < w->len(); ++i)
    {
        w->data()[i] = 0xff;
    }

    auto surface_stack = server.the_surface_stack();
    auto stream = std::make_shared<mir::compositor::Stream>();

    auto shell_surface = std::make_shared<mir::scene::BasicSurface>(
        "matt",
        mir::geometry::Rectangle{{0, 0}, buffer->size()},
        mir_pointer_unconfined,
        std::list{mir::scene::StreamInfo(stream, mir::geometry::Displacement(0, 0))},
        server.the_default_cursor_image(),
        server.the_scene_report(),
        server.the_display_configuration_observer_registrar());

    struct FooObserver : public mir::scene::NullSurfaceObserver
    {
        std::shared_ptr<mir::compositor::Stream> const stream;
        std::shared_ptr<mir::graphics::Buffer> const buffer;

        uint32_t b = 0;

        FooObserver(
            std::shared_ptr<mir::compositor::Stream> stream, std::shared_ptr<mir::graphics::Buffer> buffer) :
            stream{stream},
            buffer{buffer}
        {
        }

        void frame_posted(mir::scene::Surface const*, mir::geometry::Rectangle const&) override
        {
            auto w = std::dynamic_pointer_cast<mir::renderer::software::RWMappableBuffer>(buffer)->map_writeable();
            for (size_t i = 0; i < w->len(); i+=4)
            {
                w->data()[i + 0] = b; // r
                w->data()[i + 1] = b; // g
                w->data()[i + 2] = b; // b
                w->data()[i + 3] = b; // a
            }

            stream->submit_buffer(buffer, buffer->size(), mir::geometry::RectangleD{{0, 0}, buffer->size()});

            b = (b + 1) % 256;
            if(b == 0)
                std::cerr << "rollover\n";
        }
    };

    auto observer = std::make_shared<FooObserver>(stream, buffer);
    shell_surface->register_interest(observer);

    stream->submit_buffer(buffer, buffer->size(), mir::geometry::RectangleD{{0, 0}, buffer->size()});
    surface_stack->add_surface(shell_surface, mir::input::InputReceptionMode::normal);

    return {stream, buffer, shell_surface, observer};
}
}

void miral::LocatePointer::operator()(mir::Server& server)
{
    constexpr auto* enable_locate_pointer_opt = "enable-locate-pointer";
    constexpr auto* locate_pointer_delay_opt = "locate-pointer-delay";

    {
        auto const state = self->state.lock();
        server.add_configuration_option(enable_locate_pointer_opt, "Enable locate pointer", state->enabled);
        server.add_configuration_option(
            locate_pointer_delay_opt, "Locate pointer delay in milliseconds", static_cast<int>(state->delay.count()));
    }

    server.add_init_callback(
        [this, &server]
        {
            self->on_server_init(server);
            std::tie(self->stream, self->buffer, self->surface, self->observer) = foo(server);


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
    self->state.lock()->delay = delay;
    return *this;
}

miral::LocatePointer& miral::LocatePointer::on_locate_pointer(std::function<void(float x, float y)>&& on_locate_pointer)
{
    self->state.lock()->on_locate_pointer = std::move(on_locate_pointer);
    return *this;
}

miral::LocatePointer& miral::LocatePointer::on_enabled(std::function<void()>&& on_enabled)
{
    self->state.lock()->on_enabled = std::move(on_enabled);
    return *this;
}

miral::LocatePointer& miral::LocatePointer::on_disabled(std::function<void()>&& on_disabled)
{
    self->state.lock()->on_disabled = std::move(on_disabled);
    return *this;
}

miral::LocatePointer& miral::LocatePointer::enable()
{
    auto const state = self->state.lock();

    if (state->enabled)
        return *this;

    state->enabled = true;
    state->on_enabled();

    return *this;
}

miral::LocatePointer& miral::LocatePointer::disable()
{
    auto const state = self->state.lock();

    if (!state->enabled)
        return *this;

    state->enabled = false;
    state->locate_pointer_alarm->cancel();
    state->on_disabled();

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
