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

#include "miral/magnifier.h"

#include <utility>
#include "mir/server.h"
#include "mir/compositor/scene.h"
#include "mir/scene/basic_surface.h"
#include "mir/shell/surface_stack.h"

namespace mc = mir::compositor;
namespace mi = mir::input;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
class MagnifierSurface : public ms::BasicSurface
{
public:
    explicit MagnifierSurface(
        geom::Rectangle rect,
        std::shared_ptr<mc::Scene> const& scene,
        std::shared_ptr<mg::CursorImage> const& cursor_image,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& scene_display_configuration_observer_registrar)
        : BasicSurface(
            "magnifier",
            std::move(rect),
            mir_pointer_unconfined,
            {},
            cursor_image,
            scene_report,
            scene_display_configuration_observer_registrar),
            scene(scene)
    {
        set_depth_layer(mir_depth_layer_always_on_top);
        // scene->register_compositor(this);
    }

    bool visible() const override
    {
        return true;
    }

    mg::RenderableList generate_renderables(mc::CompositorID) const override
    {
        // auto const elements = scene->scene_elements_for(this);
        return {};
    }

private:
    std::shared_ptr<mc::Scene> const scene;
};
}

class miral::Magnifier::Self
{
public:
    Self(std::shared_ptr<msh::SurfaceStack> const& surface_stack,
        std::shared_ptr<mc::Scene> const& scene,
        std::shared_ptr<mg::CursorImage> const& cursor_image,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>> const& scene_display_configuration_observer_registrar)
        : surface_stack(surface_stack),
          magnifier_surface(std::make_shared<MagnifierSurface>(
              geom::Rectangle({0, 0}, {300, 300}),
              scene,
              cursor_image,
              scene_report,
              scene_display_configuration_observer_registrar
          ))
    {
        surface_stack->add_surface(magnifier_surface, mi::InputReceptionMode::normal);
    }

    ~Self()
    {
        surface_stack->remove_surface(magnifier_surface);
    }

private:
    std::shared_ptr<msh::SurfaceStack> const surface_stack;
    std::shared_ptr<MagnifierSurface> const magnifier_surface;
};

miral::Magnifier::Magnifier() = default;
miral::Magnifier::~Magnifier() = default;

void miral::Magnifier::operator()(mir::Server& server)
{
    server.add_init_callback([&]
    {
        self = std::make_shared<Self>(
            server.the_surface_stack(),
            server.the_scene(),
            server.the_default_cursor_image(),
            server.the_scene_report(),
            server.the_display_configuration_observer_registrar());
    });
}

