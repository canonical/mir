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

#include "basic_magnification_manager.h"

#include "mir/synchronised.h"
#include "mir/compositor/scene_element.h"
#include "mir/compositor/screen_shooter.h"
#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/frontend/surface_stack.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/renderable.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/scene.h"
#include "mir/input/seat.h"
#include "mir/scene/scene_change_notification.h"
#include "mir/renderer/sw/pixel_source.h"

#include <future>

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mrs = mir::renderer::software;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
struct State
{
    geom::PointF cursor_position;
    geom::Point capture_position;
    bool enabled_ = false;
};

class DoubleBufferStream {
public:
    DoubleBufferStream(
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        geom::Size const& size)
        : allocator(allocator),
          write_index(0),
          read_index(1)
    {
        buffers[0] = allocator->alloc_software_buffer(size, mir_pixel_format_argb_8888);
        buffers[1] = allocator->alloc_software_buffer(size, mir_pixel_format_argb_8888);
    }

    std::shared_ptr<mg::Buffer> write() {
        std::lock_guard<std::mutex> lock(mutex);
        return buffers[write_index];
    }

    void set_available()
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::swap(write_index, read_index);
    }

    std::shared_ptr<mg::Buffer> read() {
        std::lock_guard<std::mutex> lock(mutex);
        return buffers[read_index];
    }

    void resize(geom::Size const& size)
    {
        std::lock_guard<std::mutex> lock(mutex);
        buffers[0] = allocator->alloc_software_buffer(size, mir_pixel_format_argb_8888);
        buffers[1] = allocator->alloc_software_buffer(size, mir_pixel_format_argb_8888);
    }

private:
    std::shared_ptr<mg::GraphicBufferAllocator> allocator;
    std::array<std::shared_ptr<mg::Buffer>, 2> buffers;
    std::mutex mutex;
    int write_index;
    int read_index;
};

class MagnificationRenderable : public mg::Renderable
{
public:
    explicit MagnificationRenderable(
        std::shared_ptr<DoubleBufferStream> const& stream)
        : stream(stream) {}

    ID id() const override
    {
        return this;
    }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return stream->read();
    }

    geom::Rectangle screen_position() const override
    {
        return {
            rendered_position,
            stream->read()->size() * magnification
        };
    }

    geom::RectangleD src_bounds() const override
    {
        return {{0, 0}, stream->read()->size()};
    }

    std::optional<geom::Rectangle> clip_area() const override
    {
        return std::optional<geom::Rectangle>();
    }

    glm::mat4 transformation() const override
    {
        return glm::mat4{
            1, 0.0, 0.0, 0.0,
            0.0, -1, 0.0, 0.0,
            0.0, 0.0, 1, 0.0,
            0.0, 0.0, 0.0, 1.0
        };
    }

    float alpha() const override
    {
        return 1.0;
    }

    bool shaped() const override
    {
        return true;
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return std::nullopt;
    }

    geom::Point rendered_position;
    float magnification = 2.f;
    std::shared_ptr<DoubleBufferStream> stream;
};
}

class msh::BasicMagnificationManager::Self : public mi::EventFilter
{
public:
    Self(std::shared_ptr<mi::Scene> const& scene,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<compositor::ScreenShooter> const& screen_shooter,
        std::shared_ptr<frontend::SurfaceStack> const& surface_stack,
        std::shared_ptr<input::Seat> const& seat)
        : scene_change_notification(std::make_shared<ms::SceneChangeNotification>(
            [this]
            {
                auto const locked = state.lock();
                update(*locked);
            },
            [this](geom::Rectangle const& damage)
            {
                auto const locked = state.lock();
                geom::Rectangle const r(
                   locked->capture_position,
                   stream->read()->size());
                if (r.overlaps(damage))
                    update(*locked);
            }
          )),
          scene(scene),
          allocator{allocator},
          screen_shooter{screen_shooter},
          surface_stack{surface_stack},
          seat{seat},
          stream(std::make_shared<DoubleBufferStream>(
              allocator,
              geom::Size(400, 300))),
          renderable(std::make_shared<MagnificationRenderable>(stream))
    {
        auto const locked = state.lock();
        surface_stack->add_observer(scene_change_notification);
        on_cursor_moved(*locked);
    }

    ~Self() override
    {
        surface_stack->remove_observer(scene_change_notification);
    }

    bool handle(MirEvent const& event) override
    {
        if (event.type() != mir_event_type_input)
            return false;

        auto const* input_event = event.to_input();
        if (!input_event)
            return false;

        if (input_event->input_type() != mir_input_event_type_pointer)
            return false;

        auto const* pointer_event = input_event->to_pointer();
        if (!pointer_event)
            return false;

        if (pointer_event->action() != mir_pointer_action_motion)
            return false;

        auto const position_opt = pointer_event->position();
        if (!position_opt)
            return false;

        auto const locked = state.lock();
        locked->cursor_position = position_opt.value();
        on_cursor_moved(*locked);
        update(*locked);
        return false;
    }

    bool enabled(bool next)
    {
        {
            auto const locked = state.lock();
            if (next == locked->enabled_)
                return false;

            locked->enabled_ = next;
        }

        // Note that we do not have to manually call 'update'
        // after this, as adding a visualization will trigger a
        // scene change notification, which will call 'update'.
        // We make sure to give up the lock on the state before
        // or else the lock may be retaken in the scene changed
        // callback, causing a deadlock.
        if (next)
            scene->add_bottom_input_visualization(renderable);
        else
            scene->remove_input_visualization(renderable);

        return true;
    }

    geom::Size size() const
    {
        return stream->read()->size();
    }

    void resize(geom::Size const& size)
    {
        auto const locked = state.lock();
        stream->resize(size);
        on_cursor_moved(*locked);
        update(*locked);
    }

    void magnification(float magnification)
    {
        auto const locked = state.lock();
        renderable->magnification = magnification;
        on_cursor_moved(*locked);
        update(*locked);
    }

    float magnification() const
    {
        return renderable->magnification;
    }

private:
    void on_cursor_moved(State& state) const
    {
        auto const size = stream->read()->size();
        float const margin_width = static_cast<float>(size.width.as_int()) / 2.f;
        float const margin_height = static_cast<float>(size.height.as_int()) / 2.f;
        state.capture_position = {
            state.cursor_position.x.as_value() - margin_width,
            state.cursor_position.y.as_value() - margin_height
        };

        float const magnified_margin_width = margin_width * renderable->magnification;
        float const magnified_margin_height = margin_height * renderable->magnification;
        renderable->rendered_position = {
            state.cursor_position.x.as_value() - magnified_margin_width,
            state.cursor_position.y.as_value() - magnified_margin_height
        };
    }

    void update(State& state)
    {
        if (!state.enabled_)
            return;

        on_write(stream->write(), state);
    }

    void on_write(
        std::shared_ptr<mg::Buffer> const& buffer,
        State const& state)
    {
        if (is_updating)
            return;

        is_updating = true;
        geom::Rectangle const r(
            state.capture_position,
            renderable->buffer()->size());

        auto const write_buffer = mrs::as_write_mappable_buffer(buffer);
        screen_shooter->capture_with_filter(
            write_buffer,
            r,
            [renderable=renderable](std::shared_ptr<compositor::SceneElement const> const& scene_element)
            {
                return scene_element->renderable() != renderable;
            },
            false,
            [&](auto const)
            {
                stream->set_available();
                scene->emit_scene_changed();
                is_updating = false;
            });
    }

    std::shared_ptr<scene::SceneChangeNotification> scene_change_notification;
    std::shared_ptr<mi::Scene> const scene;
    std::shared_ptr<graphics::GraphicBufferAllocator> allocator;
    std::shared_ptr<compositor::ScreenShooter> screen_shooter;
    std::shared_ptr<frontend::SurfaceStack> surface_stack;
    std::shared_ptr<input::Seat> seat;

    Synchronised<State> state;
    std::shared_ptr<DoubleBufferStream> stream;
    std::shared_ptr<MagnificationRenderable> renderable;
    std::atomic<bool> is_updating = false;
};

msh::BasicMagnificationManager::BasicMagnificationManager(
    std::shared_ptr<input::CompositeEventFilter> const& filter,
    std::shared_ptr<input::Scene> const& scene,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<compositor::ScreenShooter> const& screen_shooter,
    std::shared_ptr<frontend::SurfaceStack> const& surface_stack,
    std::shared_ptr<input::Seat> const& seat)
    : self(std::make_shared<Self>(scene, allocator, screen_shooter, surface_stack, seat))
{
    filter->prepend(self);
}

bool msh::BasicMagnificationManager::enabled(bool enabled)
{
    return self->enabled(enabled);
}

void msh::BasicMagnificationManager::magnification(float magnification)
{
    self->magnification(magnification);
}

float msh::BasicMagnificationManager::magnification() const
{
    return self->magnification();
}

void msh::BasicMagnificationManager::size(geometry::Size const& size)
{
    self->resize(size);
}

geom::Size msh::BasicMagnificationManager::size() const
{
    return self->size();
}