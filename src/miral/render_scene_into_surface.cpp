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

#include "render_scene_into_surface.h"

#include "mir/executor.h"
#include "mir/server.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/screen_shooter.h"
#include "mir/compositor/screen_shooter_factory.h"
#include "mir/compositor/stream.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/scene/basic_surface.h"
#include "mir/shell/surface_stack.h"
#include "mir/renderer/sw/pixel_source.h"

#include <mutex>

namespace geom = mir::geometry;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mrs = mir::renderer::software;
namespace mi = mir::input;

namespace
{
constexpr auto DEFAULT_TRANSFORMATION = glm::mat4{
    1, 0.0, 0.0, 0.0,
    0.0, -1, 0.0, 0.0,
    0.0, 0.0, 1, 0.0,
    0.0, 0.0, 0.0, 1.0
};

class ClaimableBuffer : public mg::Buffer
{
public:
    explicit ClaimableBuffer(
        std::shared_ptr<Buffer> const& buffer,
        std::function<void()>&& release_callback)
        : buffer(buffer), release_callback(std::move(release_callback))
    {
    }

    ~ClaimableBuffer() override
    {
        release_callback();
    }

    mg::BufferID id() const override { return buffer->id(); }
    geom::Size size() const override { return buffer->size(); }
    MirPixelFormat pixel_format() const override { return buffer->pixel_format(); }
    mg::NativeBufferBase* native_buffer_base() override { return buffer->native_buffer_base(); }

private:
    std::shared_ptr<Buffer> buffer;
    std::function<void()> release_callback;
};

/// A simple buffer pool. Users provide the buffers that back the pool.
/// These buffers may be claimed later.
class BufferPool
{
public:
    using BufferBuilder = std::function<std::shared_ptr<mg::Buffer>()>;

    BufferPool(BufferBuilder&& builder_func, size_t default_size)
        : builder(std::move(builder_func))
    {
        for (size_t i = 0; i < default_size; ++i)
            buffers.push_back(builder());

        claimed.resize(default_size, false);
    }

    /// Claims the next free buffer from the pool. This may resize the buffer pool
    /// if that is required to claim a buffer.
    /// \returns A buffer from the pool
    std::shared_ptr<mg::Buffer> claim()
    {
        std::lock_guard lock{mutex};
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            if (!claimed[i])
            {
                claimed[i] = true;
                return std::make_shared<ClaimableBuffer>(buffers[i], [this, i]
                {
                    std::lock_guard lock{mutex};
                    claimed[i] = false;
                });
            }
        }

        buffers.push_back(builder());
        claimed.push_back(true);
        size_t i = buffers.size() - 1;
        return std::make_shared<ClaimableBuffer>(buffers[i], [this, i]
        {
            std::lock_guard lock{mutex};
            claimed[i] = false;
        });
    }

    void set_builder(BufferBuilder&& in_builder)
    {
        std::lock_guard lock{mutex};
        builder = std::move(in_builder);
        size_t old_size = buffers.size();
        buffers.clear();
        for (size_t i = 0; i < old_size; ++i)
            buffers.push_back(builder());

        auto last_claimed = claimed;
        claimed.resize(buffers.size(), false);

        // We keep the previously claimed buffers the same so that the release callback
        // of the ClaimableBuffer correctly unclaims it.
        for (size_t i = 0; i < last_claimed.size(); ++i)
        {
            if (last_claimed[i])
                claimed[i] = true;
        }
    }

private:
    std::mutex mutex;
    BufferBuilder builder;
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    std::vector<bool> claimed;
};

class AlwaysHasSubmittedBufferStream : public mc::Stream
{
public:
    bool has_submitted_buffer() const override
    {
        return true;
    }
};

std::list<ms::StreamInfo> create_stream_info()
{
    std::list<ms::StreamInfo> result;
    result.push_back({
        std::make_shared<AlwaysHasSubmittedBufferStream>(),
        geom::Displacement{}
    });
    return result;
}

class SceneRenderingSurface : public ms::BasicSurface
{
public:
    explicit SceneRenderingSurface(
        geom::Rectangle const& capture_rect,
        std::list<ms::StreamInfo> const& stream_info,
        std::shared_ptr<mc::Scene> const& scene,
        std::shared_ptr<mg::CursorImage> const& cursor_image,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& scene_display_configuration_observer_registrar,
        std::shared_ptr<mc::ScreenShooter> const& screen_shooter,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
        : BasicSurface(
            "internal-scene-render",
            geom::Rectangle({0, 0}, capture_rect.size),
            mir_pointer_unconfined,
            stream_info,
            cursor_image,
            scene_report,
            scene_display_configuration_observer_registrar),
            scene(scene),
          capture_rect(capture_rect),
          screen_shooter(screen_shooter),
          allocator(allocator),
          pool([allocator, capture_rect=capture_rect]
          {
              return allocator->alloc_software_buffer(capture_rect.size, mir_pixel_format_argb_8888);
          }, 2)
    {
        BasicSurface::set_transformation(DEFAULT_TRANSFORMATION);
    }

    void set_transformation(glm::mat4 const& t) override
    {
        BasicSurface::set_transformation(t * DEFAULT_TRANSFORMATION);
    }

    mg::RenderableList generate_renderables(mc::CompositorID id) const override
    {
        if (id == screen_shooter->id())
            return {};

        std::lock_guard lock{mutex};
        auto buffer = pool.claim();
        auto const mapping = mrs::as_write_mappable_buffer(buffer);
        screen_shooter->capture(
            mapping,
            capture_rect,
            overlay_cursor,
            [](auto const&) {});
        get_streams().begin()->stream->submit_buffer(
            buffer, capture_rect.size, geom::RectangleD({0, 0}, capture_rect.size));
        return BasicSurface::generate_renderables(id);
    }

    bool input_area_contains(mir::geometry::Point const&) const override
    {
        return false;
    }

    void set_capture_area(geom::Rectangle const& next_capture_rect)
    {
        std::lock_guard lock{mutex};

        if (capture_rect.size != next_capture_rect.size)
        {
            pool.set_builder([allocator=allocator, next_capture_rect=next_capture_rect]
            {
                return allocator->alloc_software_buffer(next_capture_rect.size, mir_pixel_format_argb_8888);
            });
        }

        move_to(next_capture_rect.top_left);
        resize(next_capture_rect.size);
        capture_rect = next_capture_rect;
    }

    void set_overlay_cursor(bool next_overlay_cursor)
    {
        std::lock_guard lock{mutex};
        overlay_cursor = next_overlay_cursor;
    }

private:
    std::mutex mutable mutex;
    std::shared_ptr<mc::Scene> const scene;
    geom::Rectangle capture_rect;
    bool overlay_cursor = false;
    std::shared_ptr<mc::ScreenShooter> screen_shooter;
    std::shared_ptr<mg::GraphicBufferAllocator> allocator;
    BufferPool mutable pool;
};
}

class miral::RenderSceneIntoSurface::Self
{
public:
    explicit Self(geom::Rectangle const& rect)
        : initial_capture_area(rect)
    {
    }

    void init(mir::Server& server)
    {
        std::lock_guard lock{mutex};
        surface = std::make_shared<SceneRenderingSurface>(
            initial_capture_area,
            create_stream_info(),
            server.the_scene(),
            server.the_default_cursor_image(),
            server.the_scene_report(),
            server.the_display_configuration_observer_registrar(),
            server.the_screen_shooter_factory()->create(mir::immediate_executor),
            server.the_buffer_allocator());
        server.the_surface_stack()->add_surface(surface, mi::InputReceptionMode::normal);
        on_surface_ready(surface);
        surface_stack = server.the_surface_stack();

        server.add_stop_callback([this]
        {
            std::lock_guard lock{mutex};
            if (auto const locked = surface_stack.lock())
                locked->remove_surface(surface);
            surface = nullptr;
        });
    }

    void set_capture_area(geom::Rectangle const& area)
    {
        std::lock_guard lock{mutex};
        initial_capture_area = area;

        if (surface)
            surface->set_capture_area(area);
    }

    geom::Rectangle capture_area()
    {
        std::lock_guard lock{mutex};
        return initial_capture_area;
    }

    void set_overlay_cursor(bool overlay_cursor)
    {
        std::lock_guard lock{mutex};
        initial_overlay_cursor = overlay_cursor;
        if (surface)
            surface->set_overlay_cursor(overlay_cursor);
    }

    void set_surface_ready_callback(std::function<void(std::shared_ptr<mir::scene::Surface> const&)>&& callback)
    {
        std::lock_guard lock{mutex};
        on_surface_ready = std::move(callback);
        if (surface)
            on_surface_ready(surface);
    }

private:
    std::mutex mutex;
    std::function<void(std::shared_ptr<mir::scene::Surface> const&)> on_surface_ready{[](auto const&){}};
    geom::Rectangle initial_capture_area;
    bool initial_overlay_cursor = false;
    std::shared_ptr<SceneRenderingSurface> surface;
    std::weak_ptr<msh::SurfaceStack> surface_stack;
};

miral::RenderSceneIntoSurface::RenderSceneIntoSurface()
    : self(std::make_shared<Self>(geom::Rectangle({0, 0}, geom::Size(200, 200))))
{
}

miral::RenderSceneIntoSurface::~RenderSceneIntoSurface() = default;

miral::RenderSceneIntoSurface& miral::RenderSceneIntoSurface::capture_area(geom::Rectangle const& area)
{
    self->set_capture_area(area);
    return *this;
}

geom::Rectangle miral::RenderSceneIntoSurface::capture_area() const
{
    return self->capture_area();
}

miral::RenderSceneIntoSurface& miral::RenderSceneIntoSurface::overlay_cursor(bool overlay_cursor)
{
    self->set_overlay_cursor(overlay_cursor);
    return *this;
}

miral::RenderSceneIntoSurface& miral::RenderSceneIntoSurface::on_surface_ready(
    std::function<void(std::shared_ptr<mir::scene::Surface> const&)>&& callback)
{
    self->set_surface_ready_callback(std::move(callback));
    return *this;
}

void miral::RenderSceneIntoSurface::operator()(mir::Server& server)
{
    server.add_init_callback([&]
    {
        self->init(server);
    });
}

