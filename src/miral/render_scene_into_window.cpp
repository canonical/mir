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
class SceneRenderingRenderable : public mg::Renderable
{
public:
    SceneRenderingRenderable(
        ms::Surface const* surface,
        std::shared_ptr<mg::Buffer> const& buffer,
        geom::Rectangle const& screen_position)
        : surface(surface),
          buffer_(buffer),
          screen_position_(screen_position)
    {
    }

    ID id() const override
    {
        return this;
    }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return buffer_;
    }

    geom::Rectangle screen_position() const override
    {
        return screen_position_;
    }

    geom::RectangleD src_bounds() const override
    {
        return geom::RectangleD{{0, 0}, buffer_->size()};
    }

    std::optional<geom::Rectangle> clip_area() const override
    {
        return std::nullopt;
    }

    float alpha() const override
    {
        return 1.f;
    }

    glm::mat4 transformation() const override
    {
        // The transformation must always invert the Y as this is a GL texture.
        return glm::mat4{
            1, 0.0, 0.0, 0.0,
            0.0, -1, 0.0, 0.0,
            0.0, 0.0, 1, 0.0,
            0.0, 0.0, 0.0, 1.0
        };
    }

    bool shaped() const override
    {
        return true;
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return surface;
    }

private:
    ms::Surface const* surface;
    std::shared_ptr<mg::Buffer> const buffer_;
    geom::Rectangle const screen_position_;
};

class SceneRenderingSurface : public ms::BasicSurface
{
public:
    explicit SceneRenderingSurface(
        geom::Rectangle const& rect,
        std::shared_ptr<mc::Scene> const& scene,
        std::shared_ptr<mg::CursorImage> const& cursor_image,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& scene_display_configuration_observer_registrar,
        std::shared_ptr<mc::ScreenShooter> const& screen_shooter,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
        : BasicSurface(
            "magnifier",
            rect,
            mir_pointer_unconfined,
            {},
            cursor_image,
            scene_report,
            scene_display_configuration_observer_registrar),
            scene(scene),
          rect(rect),
          screen_shooter(screen_shooter),
          allocator(allocator),
          buffer(allocator->alloc_software_buffer(rect.size, mir_pixel_format_argb_8888))
    {
    }

    ~SceneRenderingSurface() override
    {
    }

    bool visible() const override
    {
        return true;
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
        return mg::RenderableList{
            std::make_shared<SceneRenderingRenderable>(this, buffer, geom::Rectangle({0, 0}, rect.size))
        };
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
        : surface(std::make_shared<SceneRenderingSurface>(
            rect,
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

