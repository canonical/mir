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
#include "mir/compositor/stream.h"
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
#include "mir/log.h"

#include <condition_variable>

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mc = mir::compositor;
namespace mrs = mir::renderer::software;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
class NotifyingBuffer : public mg::Buffer
{
public:
    NotifyingBuffer(
        std::shared_ptr<Buffer> const& wrapped,
        std::function<void()>&& on_release)
        : wrapped(wrapped),
          on_release(std::move(on_release))
        {}

    ~NotifyingBuffer() override
    {
        on_release();
    }

    mg::BufferID id() const override
    {
        return wrapped->id();
    }

    geom::Size size() const override
    {
        return wrapped->size();
    }

    MirPixelFormat pixel_format() const override
    {
        return wrapped->pixel_format();
    }

    mg::NativeBufferBase* native_buffer_base() override
    {
        return wrapped->native_buffer_base();
    }

    std::shared_ptr<Buffer> buffer() const
    {
        return wrapped;
    }

private:
    std::shared_ptr<Buffer> const wrapped;
    std::function<void()> on_release;
};

class DoubleBufferStream
{
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
        submit();
    }

    /// Grabs the current write buffer. When the caller stops using the buffer,
    /// the content of the buffer will be swapped with the current read buffer.
    /// Callers may not have more than one reference to the active write buffer.
    /// Asking for one will return std::nullopt.
    std::optional<std::unique_ptr<NotifyingBuffer>> write_buffer()
    {
        std::lock_guard lock(mutex);
        if (is_writing)
            return std::nullopt;

        is_writing = true;
        return std::make_unique<NotifyingBuffer>(
            buffers[write_index],
            [this]
            {
                std::unique_lock lock(mutex);
                is_writing = false;
                cv.wait(lock, [this] { return !is_reading; });
                std::swap(write_index, read_index);
                submit();
                lock.unlock();
            });
    }

    std::shared_ptr<mg::Buffer> read()
    {
        // We want to avoid writing to a buffer that we are currently reading from. To do
        // this, we set a flag that tells us if that buffer is currently being read
        // or not. We then return the buffer wrapped in a [NotifyingBuffer]. When the
        // notifying buffer deconstructs, it will set the flag to false and notify anyone
        // who was waiting on it. For example, the deconstructor of the write buffer
        // may be waiting on the flag. Once the flag is triggered, the read/write indices
        // are swapped and a new buffer is submitted to the stream.
        std::lock_guard lock(mutex);
        if (is_reading)
        {
            if (auto locked_read_buffer = read_buffer.lock())
                return read_buffer.lock();
        }

        is_reading = true;
        auto next_submission = stream.next_submission_for_compositor(this)->claim_buffer();
        auto retval = std::make_shared<NotifyingBuffer>(
            next_submission,
            [this]
            {
                std::lock_guard lock(mutex);
                is_reading = false;
                read_buffer.reset();
                cv.notify_all();
            });
        read_buffer = retval;
        return retval;
    }

    void resize(geom::Size const& size)
    {
        // We are either going to commit this resize:
        // 1. Immediately because we are neither reading nor writing
        // 2. After the next [write] has completed. At that point, we will
        //    have rendered a new buffer and submitted it to the stream.
        //    This means that the next read will return the previously-sized
        //    buffer and we can start writing to our newly-sized buffers.
        std::unique_lock lock(mutex);
        next_size = size;
        if (!is_reading && !is_writing)
            try_resize();
    }

private:
    void try_resize()
    {
        if (!next_size)
            return;

        buffers[0] = allocator->alloc_software_buffer(next_size.value(), mir_pixel_format_argb_8888);
        buffers[1] = allocator->alloc_software_buffer(next_size.value(), mir_pixel_format_argb_8888);
        next_size.reset();
    }

    void submit()
    {
        stream.submit_buffer(
            buffers[read_index],
            buffers[read_index]->size(),
            geom::RectangleD(
                {0, 0},
                buffers[read_index]->size()));
    }

    std::shared_ptr<mg::GraphicBufferAllocator> allocator;
    std::array<std::shared_ptr<mg::Buffer>, 2> buffers;
    mc::Stream stream;
    std::mutex mutex;
    std::condition_variable cv;
    bool is_writing = false;
    bool is_reading = false;
    std::weak_ptr<NotifyingBuffer> read_buffer;
    int write_index;
    int read_index;
    std::optional<geom::Size> next_size;
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
    struct State
    {
        geom::PointF cursor_position;
        geom::Point capture_position;
        bool enabled_ = false;
    };

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

        if (is_updating.exchange(true))
            return;

        // If we can grab the write buffer, then we can capture the next render.
        // Otherwise, we still have a render outstanding.
        if (auto buffer = stream->write_buffer())
            active_write_buffer = std::move(buffer.value());
        else
        {
            mir::log_error("Unexpectedly unable to grab a write buffer");
            return;
        }

        auto const write_buffer = mrs::as_write_mappable_buffer(
            active_write_buffer->buffer());
        screen_shooter->capture_with_filter(
            write_buffer,
            geom::Rectangle(
                state.capture_position,
                renderable->buffer()->size()),
            [renderable=renderable](std::shared_ptr<compositor::SceneElement const> const& scene_element)
            {
                return scene_element->renderable() != renderable;
            },
            false,
            [this](auto const)
            {
                // By nullifying the write buffer, we lose a reference to it and thus
                // trigger its destructor (and a swap!).
                active_write_buffer = nullptr;

                // After swapping, we emit the scene change. It is important that
                // [is_updating] is set to false afterwards so that the magnification
                // manager itself does not respond to this scene change.
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
    std::unique_ptr<NotifyingBuffer> active_write_buffer;
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