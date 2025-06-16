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
          buffer(allocator->alloc_software_buffer(capture_rect.size, mir_pixel_format_argb_8888))
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

    void set_capture_area(geom::Rectangle const& area)
    {
        std::lock_guard lock{mutex};
        capture_rect.top_left = area.top_left;

        if (capture_rect.size != buffer->size())
            buffer = allocator->alloc_software_buffer(capture_rect.size, mir_pixel_format_argb_8888);
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
    std::shared_ptr<mg::Buffer> buffer;
};
}

class miral::RenderSceneIntoSurface::Self
{
public:
    explicit Self(geom::Rectangle const& rect)
        : initial_capture_area(rect)
    {
    }

    ~Self()
    {
        if (surface&& surface_stack)
            surface_stack->remove_surface(surface);
    }

    void operator()(mir::Server& server)
    {
        surface_stack = server.the_surface_stack();
        surface = std::make_shared<SceneRenderingSurface>(
            initial_capture_area,
            create_stream_info(),
            server.the_scene(),
            server.the_default_cursor_image(),
            server.the_scene_report(),
            server.the_display_configuration_observer_registrar(),
            server.the_screen_shooter_factory()->create(mir::immediate_executor),
            server.the_buffer_allocator());
        surface_stack->add_surface(surface, mi::InputReceptionMode::normal);
        on_surface_ready(surface);
    }

    void set_capture_area(geom::Rectangle const& area)
    {
        initial_capture_area = area;

        if (surface)
            surface->set_capture_area(area);
    }

    void set_overlay_cursor(bool overlay_cursor)
    {
        initial_overlay_cursor = overlay_cursor;
        if (surface)
            surface->set_overlay_cursor(overlay_cursor);
    }

    void set_surface_ready_callback(std::function<void(std::shared_ptr<mir::scene::Surface> const&)>&& callback)
    {
        on_surface_ready = std::move(callback);
        if (surface)
            on_surface_ready(surface);
    }

private:
    std::function<void(std::shared_ptr<mir::scene::Surface> const&)> on_surface_ready;
    geom::Rectangle initial_capture_area;
    bool initial_overlay_cursor = false;
    std::shared_ptr<SceneRenderingSurface> surface;
    std::shared_ptr<msh::SurfaceStack> surface_stack;
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
        self->operator()(server);
    });
}

