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

#include "mir/server.h"
#include "mir/synchronised.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/scene_element.h"
#include "mir/scene/basic_surface.h"
#include "mir/shell/surface_stack.h"

#include <glm/gtc/matrix_transform.hpp>

namespace mc = mir::compositor;
namespace mi = mir::input;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
class MagnifierRenderable : public mg::Renderable
{
public:
    MagnifierRenderable(
        std::shared_ptr<Renderable> const& wrapped,
        float magnification,
        geom::Rectangle const& rect)
        : wrapped(wrapped),
          magnification(magnification),
          rect(rect)
    {
    }

    ID id() const override
    {
        return wrapped->id();
    }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return wrapped->buffer();
    }

    geom::Rectangle screen_position() const override
    {
        return wrapped->screen_position();
    }

    geom::RectangleD src_bounds() const override
    {
        return wrapped->src_bounds();
    }

    std::optional<geom::Rectangle> clip_area() const override
    {
        if (auto const clip = wrapped->clip_area())
        {
            if (rect.contains(clip.value()))
                return clip.value();
        }

        return rect;
    }

    float alpha() const override
    {
        return wrapped->alpha();
    }

    glm::mat4 transformation() const override
    {
        return glm::scale(wrapped->transformation(), glm::vec3(magnification, magnification, 1));
    }

    bool shaped() const override
    {
        return wrapped->shaped();
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return wrapped->surface_if_any();
    }

private:
    std::shared_ptr<Renderable> wrapped;
    float magnification;
    geom::Rectangle rect;
};

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
        scene->register_compositor(this);
        state.lock()->rect = rect;
    }

    ~MagnifierSurface() override
    {
        scene->unregister_compositor(this);
    }

    bool visible() const override
    {
        return state.lock()->visible;
    }

    mg::RenderableList generate_renderables(mc::CompositorID id) const override
    {
        if (id == this)
            return {};

        mg::RenderableList renderables;
        auto const elements = scene->scene_elements_for(this);
        auto const rect = state.lock()->rect;
        auto const scale = state.lock()->scale;
        for (auto const& element : elements)
        {
            if (element->renderable()->screen_position().overlaps(rect))
            {
                renderables.push_back(std::make_shared<MagnifierRenderable>(
                    element->renderable(),
                    scale,
                    rect));
            }
        }
        return renderables;
    }

private:
    struct State
    {
        bool visible = true;
        geom::Rectangle rect = geom::Rectangle({0, 0}, {0, 0});
        float scale = 2.f;
    };

    std::shared_ptr<mc::Scene> const scene;
    mir::Synchronised<State> state;
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
              geom::Rectangle({0, 0}, {400, 300}),
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

