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

#include "default_display_buffer_compositor.h"

#include "mir/compositor/rendering_operator.h"
#include "mir/compositor/overlay_renderer.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/compositing_criteria.h"
#include "mir/graphics/display_buffer.h"
#include "mir/surfaces/buffer_stream.h"

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
        return info.should_be_rendered_in(enclosing_region);
    }

    mir::geometry::Rectangle const& enclosing_region;
};


struct BypassFilter : public mc::FilterForScene
{
    BypassFilter(const mg::DisplayBuffer &display_buffer)
    {
        const mir::geometry::Rectangle &rect = display_buffer.view_area();
        int width = rect.size.width.as_int();
        int height = rect.size.height.as_int();

        /*
         * For a surface to exactly fit the display_buffer, its transformation
         * will look exactly like this:
         */
        fullscreen[0][0] = width;
        fullscreen[0][1] = 0.0f;
        fullscreen[0][2] = 0.0f;
        fullscreen[0][3] = 0.0f;

        fullscreen[1][0] = 0.0f;
        fullscreen[1][1] = height;
        fullscreen[1][2] = 0.0f;
        fullscreen[1][3] = 0.0f;

        fullscreen[2][0] = 0.0f;
        fullscreen[2][1] = 0.0f;
        fullscreen[2][2] = 0.0f;
        fullscreen[2][3] = 0.0f;

        fullscreen[3][0] = rect.top_left.x.as_int() + width / 2;
        fullscreen[3][1] = rect.top_left.y.as_int() + height / 2;
        fullscreen[3][2] = 0.0f;
        fullscreen[3][3] = 1.0f;
    }

    bool operator()(mc::CompositingCriteria const& criteria)
    {
        fullscreen_on_top = criteria.transformation() == fullscreen;
        return fullscreen_on_top;
    }

    bool fullscreen_on_top = false;
    glm::mat4 fullscreen;
};

struct BypassSearch : public mc::OperatorForScene
{
    void operator()(mc::CompositingCriteria const&,
                    mir::surfaces::BufferStream& stream)
    {
        result = &stream;
    }

    mir::surfaces::BufferStream *result = nullptr;
};

}

mc::DefaultDisplayBufferCompositor::DefaultDisplayBufferCompositor(
    mg::DisplayBuffer& display_buffer,
    std::shared_ptr<mc::Scene> const& scene,
    std::shared_ptr<mc::Renderer> const& renderer,
    std::shared_ptr<mc::OverlayRenderer> const& overlay_renderer)
    : mc::BasicDisplayBufferCompositor{display_buffer},
      scene{scene},
      renderer{renderer},
      overlay_renderer{overlay_renderer}
{
}

void mc::DefaultDisplayBufferCompositor::composite()
{
    bool bypassed = false;

    if (display_buffer.can_bypass())
    {
        BypassFilter filter(display_buffer);
        BypassSearch search;

        // It would be *really* nice if Scene had an iterator to simplify this
        scene->for_each_if(filter, search);

        if (filter.fullscreen_on_top && search.result)
        {
            display_buffer.post_update(
                search.result->lock_compositor_buffer());
            bypassed = true;
        }
    }

    if (!bypassed)
        mc::BasicDisplayBufferCompositor::composite();
}

void mc::DefaultDisplayBufferCompositor::compose(
    mir::geometry::Rectangle const& view_area,
    std::function<void(std::shared_ptr<void> const&)> save_resource)
{
    renderer->clear();

    mc::RenderingOperator applicator(*renderer, save_resource);
    FilterForVisibleSceneInRegion selector(view_area);
    scene->for_each_if(selector, applicator);

    overlay_renderer->render(view_area, save_resource);
}
