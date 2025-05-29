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

#include "miral/render_scene_into_window.h"

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
        geom::Rectangle const& rect,
        std::list<ms::StreamInfo> const& stream_info,
        std::shared_ptr<mc::Scene> const& scene,
        std::shared_ptr<mg::CursorImage> const& cursor_image,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& scene_display_configuration_observer_registrar,
        std::shared_ptr<mc::ScreenShooter> const& screen_shooter,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
        : BasicSurface(
            "internal-scene-render",
            rect,
            mir_pointer_unconfined,
            stream_info,
            cursor_image,
            scene_report,
            scene_display_configuration_observer_registrar),
            scene(scene),
          rect(rect),
          screen_shooter(screen_shooter),
          allocator(allocator),
          buffer(allocator->alloc_software_buffer(rect.size, mir_pixel_format_argb_8888))
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

        auto const mapping = mrs::as_write_mappable_buffer(buffer);
        screen_shooter->capture(
            mapping,
            rect,
            [](auto const&) {});
        get_streams().begin()->stream->submit_buffer(buffer, rect.size, geom::RectangleD({0, 0}, rect.size));
        return BasicSurface::generate_renderables(id);
    }

private:
    std::shared_ptr<mc::Scene> const scene;
    geom::Rectangle rect;
    std::shared_ptr<mc::ScreenShooter> screen_shooter;
    std::shared_ptr<mg::GraphicBufferAllocator> allocator;
    std::shared_ptr<mg::Buffer> buffer;
};
}

class miral::RenderSceneIntoWindow::Self
{
public:
    Self(
        Rectangle const& rect,
        std::shared_ptr<mc::Scene> const& scene,
        std::shared_ptr<mg::CursorImage> const& cursor_image,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>> const& scene_display_configuration_observer_registrar,
        std::shared_ptr<mc::ScreenShooter> const& screen_shooter,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<msh::SurfaceStack> const& surface_stack)
        :  stream_info(create_stream_info()),
           surface(std::make_shared<SceneRenderingSurface>(
               rect,
               stream_info,
               scene,
               cursor_image,
               scene_report,
               scene_display_configuration_observer_registrar,
               screen_shooter,
               allocator)),
          surface_stack(surface_stack)
    {
        surface_stack->add_surface(surface, mi::InputReceptionMode::normal);
    }

    ~Self()
    {
        surface_stack->remove_surface(surface);
    }

private:
    std::list<ms::StreamInfo> stream_info;
    std::shared_ptr<SceneRenderingSurface> surface;
    std::shared_ptr<msh::SurfaceStack> surface_stack;
};

miral::RenderSceneIntoWindow::RenderSceneIntoWindow() = default;
miral::RenderSceneIntoWindow::~RenderSceneIntoWindow() = default;

void miral::RenderSceneIntoWindow::operator()(mir::Server& server)
{
    server.add_init_callback([&]
    {
        self = std::make_shared<Self>(
            Rectangle({0, 0}, geom::Size(200, 200)),
            server.the_scene(),
            server.the_default_cursor_image(),
            server.the_scene_report(),
            server.the_display_configuration_observer_registrar(),
            server.the_screen_shooter_factory()->create(mir::immediate_executor),
            server.the_buffer_allocator(),
            server.the_surface_stack());
    });
}

