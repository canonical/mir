/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/compositor/default_display_buffer_compositor_factory.h"
#include "mir/compositor/basic_display_buffer_compositor.h"

#include "mir/compositor/rendering_operator.h"
#include "mir/compositor/overlay_renderer.h"
#include "mir/compositor/scene.h"
#include "mir/geometry/rectangle.h"
#include "mir/compositor/renderer_factory.h"
#include "mir/compositor/compositing_criteria.h"
#include "mir/graphics/display_buffer.h"

#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{

struct FilterForVisibleSceneInRegion : public mc::FilterForScene
{
    FilterForVisibleSceneInRegion(mir::geometry::Rectangle const& enclosing_region)
        : enclosing_region(enclosing_region)
    {
    }
    bool operator()(mc::CompositingCriteria const& info)
    {
        return info.should_be_rendered();
    }

    mir::geometry::Rectangle const& enclosing_region;
};

class DefaultDisplayBufferCompositor : public mc::BasicDisplayBufferCompositor
{
public:
    DefaultDisplayBufferCompositor(
        mg::DisplayBuffer& display_buffer,
        std::shared_ptr<mc::Scene> const& scene,
        std::shared_ptr<mc::Renderer> const& renderer,
        std::shared_ptr<mc::OverlayRenderer> const& overlay_renderer)
        : mc::BasicDisplayBufferCompositor{display_buffer},
          scene{scene},
          renderer{renderer},
          overlay_renderer{overlay_renderer}
    {
        assert(scene);
        assert(renderer);
        assert(overlay_renderer);
    }

    void compose(
        mir::geometry::Rectangle const& view_area,
        std::function<void(std::shared_ptr<void> const&)> save_resource) override
    {
        renderer->clear();

        mc::RenderingOperator applicator(*renderer, save_resource);
        FilterForVisibleSceneInRegion selector(view_area);
        scene->for_each_if(selector, applicator);

        overlay_renderer->render(view_area, save_resource);
    }

private:
    std::shared_ptr<mc::Scene> const scene;
    std::shared_ptr<mc::Renderer> const renderer;
    std::shared_ptr<mc::OverlayRenderer> const overlay_renderer;
};


}


mc::DefaultDisplayBufferCompositorFactory::DefaultDisplayBufferCompositorFactory(
    std::shared_ptr<mc::Scene> const& scene,
    std::shared_ptr<mc::RendererFactory> const& renderer_factory,
    std::shared_ptr<mc::OverlayRenderer> const& overlay_renderer)
    : scene{scene},
      renderer_factory{renderer_factory},
      overlay_renderer{overlay_renderer}
{
}

std::unique_ptr<mc::DisplayBufferCompositor>
mc::DefaultDisplayBufferCompositorFactory::create_compositor_for(graphics::DisplayBuffer& display_buffer)
{
    auto renderer = renderer_factory->create_renderer_for(display_buffer.view_area());
    auto raw = new DefaultDisplayBufferCompositor{display_buffer, scene, std::move(renderer),
                                                  overlay_renderer};

    return std::unique_ptr<DisplayBufferCompositor>(raw);
}
