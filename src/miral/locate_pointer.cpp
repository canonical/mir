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
#include "mir/events/pointer_event.h"
#include "mir/geometry/forward.h"
#include "mir/graphics/animation_driver.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/event_filter.h"
#include "mir/main_loop.h"
#include "mir/renderer/sw/pixel_source.h"
#include "mir/scene/basic_surface.h"
#include "mir/scene/surface.h"
#include "mir/server.h"
#include "mir/shell/surface_stack.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"

#include <list>

namespace ms = mir::scene;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

class BasicSurfaceClickCrashBandAid : public ms::BasicSurface
{
public:
    BasicSurfaceClickCrashBandAid(
        geom::Size size,
        std::shared_ptr<mc::Stream> stream,
        std::shared_ptr<mg::CursorImage> default_cursor_image,
        std::shared_ptr<ms::SceneReport> scene_report,
        std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>>
            display_configuration_observer_registrar) :
        ms::BasicSurface{
            "locate-pointer-graphical-feedback",
            geom::Rectangle{{-size.width.as_int() / 2, -size.height.as_int() / 2}, size},
            mir_pointer_unconfined,
            std::list{ms::StreamInfo(stream, geom::Displacement(0, 0))},
            std::move(default_cursor_image),
            std::move(scene_report),
            std::move(display_configuration_observer_registrar)}
    {
    }

    bool input_area_contains(geom::Point const&) const override
    {
        return false;
    }
};

struct miral::LocatePointer::Self
{
    struct PointerPositionRecorder : public mir::input::EventFilter
    {
        struct State
        {
            geom::PointF cursor_position{0.0f, 0.0f}; // Assumes the cursor always starts at (0, 0)

            std::weak_ptr<ms::BasicSurface> surface;
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
            {
                auto const s = state.lock();
                s->cursor_position = *position;

                if(auto surface = s->surface.lock())
                {
                    auto const r = surface->content_size().width.as_value() / 2;
                    geom::Point p = {position->x.as_value() - r, position->y.as_value() - r};
                    surface->move_to(p);
                }
            }

            return false;
        }

        mir::Synchronised<State> state;
    };

    Self(bool enable_by_default) :
        state{State{
            enable_by_default,
            std::make_shared<Self::PointerPositionRecorder>(),
            [this](auto, auto)
            {
                this->observer->start_animation();
            }}}
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
        State(
            bool enabled,
            std::shared_ptr<PointerPositionRecorder> const& position_recorder,
            std::function<void(float, float)> on_locate_pointer) :
            enabled{enabled},
            pointer_position_recorder{position_recorder},
            on_locate_pointer{on_locate_pointer}
        {
        }

        bool enabled;
        std::shared_ptr<PointerPositionRecorder> const pointer_position_recorder;

        std::unique_ptr<mir::time::Alarm> locate_pointer_alarm;
        std::function<void(float, float)> on_locate_pointer;
        std::chrono::milliseconds delay{500};
    };

    struct CircleDrawingObserver : public mg::AnimationObserver
    {
        std::shared_ptr<mir::compositor::Stream> const stream;
        std::shared_ptr<mg::Buffer> const buffer;
        std::shared_ptr<BasicSurfaceClickCrashBandAid> const shell_surface;
        std::shared_ptr<mir::shell::SurfaceStack> const surface_stack;

        struct State 
        {
            bool active{false};
            float t{0};
            uint32_t radius{0};
        };

        mir::Synchronised<State> state;

        uint32_t const max_radius;
        auto static constexpr animation_length = std::chrono::milliseconds{1500};

        CircleDrawingObserver(
            std::shared_ptr<mir::compositor::Stream> stream,
            std::shared_ptr<mg::Buffer> buffer,
            std::shared_ptr<BasicSurfaceClickCrashBandAid> shell_surface,
            std::shared_ptr<mir::shell::SurfaceStack> surface_stack) :
            stream{std::move(stream)},
            buffer{std::move(buffer)},
            shell_surface{std::move(shell_surface)},
            surface_stack{std::move(surface_stack)},
            max_radius{static_cast<uint32_t>(
                std::min(this->buffer->size().width.as_value(), this->buffer->size().height.as_value()) / 2)}
        {
        }

        void on_vsync(std::chrono::milliseconds dt) override
        {
            auto s = state.lock();

            // Don't submit any frames if we're not active
            if(!s->active)
                return;

            if (s->t > 1.0f)
            {
                s->active = false;
                surface_stack->remove_surface(shell_surface);
                return;
            }

            draw_circle(0xAA, 0xAA, 0xAA, 0x99, s->radius);

            // Force the maximum frametime to be 16ms. This is to account for
            // when rendering idles (which would result in a huge dt)
            s->t += static_cast<float>(std::clamp(static_cast<int>(dt.count()), 0, 16)) / animation_length.count();

            // Sawtooth 3 times
            s->radius = static_cast<uint32_t>((std::lerp(0u, max_radius * 3, s->t))) % max_radius;
        }

        void draw_circle(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int radius)
        {
            auto w = std::dynamic_pointer_cast<mir::renderer::software::RWMappableBuffer>(buffer)->map_writeable();
            auto const center = geom::Point{max_radius, max_radius};
            for (size_t i = 0; i < w->len(); i += 4)
            {
                auto index = i / 4;
                auto p = geom::Point{index % (2 * max_radius), index / (2 * max_radius)};
                auto dist = (p - center).length_squared();

                auto circle = [dist, radius](auto value)
                {
                    return dist < radius * radius ? value : 0;
                };

                w->data()[i + 0] = circle(r); // r
                w->data()[i + 1] = circle(g); // g
                w->data()[i + 2] = circle(b); // b
                w->data()[i + 3] = circle(a);
            }

            stream->submit_buffer(buffer, buffer->size(), geom::RectangleD{{0, 0}, buffer->size()});
        }

        void start_animation()
        {
            auto s = state.lock();
            s->t = 0;
            s->radius = 0;

            if (!s->active)
            {
                // Clear the buffer before adding it. Otherwise, it will
                // momentarily show the last drawn into it.
                {
                    auto w =
                        std::dynamic_pointer_cast<mir::renderer::software::RWMappableBuffer>(buffer)->map_writeable();
                    std::memset(w->data(), 0, w->len());
                }
                surface_stack->add_surface(shell_surface, mir::input::InputReceptionMode::normal);
            }

            s->active = true;
            draw_circle(0, 0, 0, 0, s->radius);
        }
    };

    mir::Synchronised<State> state;
    std::shared_ptr<CircleDrawingObserver> observer;
};

miral::LocatePointer::LocatePointer(bool enabled_by_default) :
    self(std::make_shared<Self>(enabled_by_default))
{
}

void miral::LocatePointer::operator()(mir::Server& server)
{
    server.add_init_callback(
        [this, &server]
        {
            self->on_server_init(server);
            {
                auto const size = geom::Size(100, 100);
                auto stream = std::make_shared<mc::Stream>();

                auto shell_surface = std::make_shared<BasicSurfaceClickCrashBandAid>(
                    size,
                    stream,
                    server.the_default_cursor_image(),
                    server.the_scene_report(),
                    server.the_display_configuration_observer_registrar());

                auto buffer = server.the_buffer_allocator()->alloc_software_buffer(size, mir_pixel_format_abgr_8888);
                self->observer = std::make_shared<Self::CircleDrawingObserver>(stream, buffer, shell_surface, server.the_surface_stack());
                self->state.lock()->pointer_position_recorder->state.lock()->surface = shell_surface;

                server.the_animation_driver()->register_interest(self->observer);
            }

            if (self->state.lock()->enabled)
                enable();
            else
                disable();
        });

    // Free the observer to free the buffer it holds a reference to
    server.add_stop_callback([this] { self->observer.reset(); });
}

miral::LocatePointer& miral::LocatePointer::delay(std::chrono::milliseconds delay)
{
    self->state.lock()->delay = delay;
    return *this;
}

miral::LocatePointer& miral::LocatePointer::on_locate_pointer(std::function<void(float x, float y)>&& on_locate_pointer)
{
    self->state.lock()->on_locate_pointer = [on_locate_pointer = std::move(on_locate_pointer), this](auto x, auto y)
    {
        self->observer->start_animation();
        on_locate_pointer(x, y);
    };
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
