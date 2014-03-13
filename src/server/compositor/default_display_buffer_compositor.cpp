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
 *              Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "default_display_buffer_compositor.h"

#include "rendering_operator.h"
#include "mir/compositor/scene.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/buffer.h"
#include "mir/compositor/buffer_stream.h"
#include "bypass.h"
#include "occlusion.h"
#include <mutex>
#include <cstdlib>
#include <vector>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{

struct FilterForVisibleSceneInRegion : public mc::FilterForScene
{
    FilterForVisibleSceneInRegion(
        mir::geometry::Rectangle const& enclosing_region,
        mc::OcclusionMatch const& occlusions)
        : enclosing_region(enclosing_region),
          occlusions(occlusions)
    {
    }
    bool operator()(mg::Renderable const& r)
    {
        return r.should_be_rendered_in(enclosing_region) &&
               !occlusions.occluded(r);
    }

    mir::geometry::Rectangle const& enclosing_region;
    mc::OcclusionMatch const& occlusions;
};

std::mutex global_frameno_lock;
unsigned long global_frameno = 0;

bool wrapped_greater_or_equal(unsigned long a, unsigned long b)
{
    return (a - b) < (~0UL / 2UL);
}

}

mc::DefaultDisplayBufferCompositor::DefaultDisplayBufferCompositor(
    mg::DisplayBuffer& display_buffer,
    std::shared_ptr<mc::Scene> const& scene,
    std::shared_ptr<mc::Renderer> const& renderer,
    std::shared_ptr<mc::CompositorReport> const& report)
    : display_buffer(display_buffer),
      scene{scene},
      renderer{renderer},
      report{report},
      local_frameno{global_frameno}
{
}


bool mc::DefaultDisplayBufferCompositor::composite()
{
    report->began_frame(this);

    /*
     * Increment frame counts for each tick of the fastest instance of
     * DefaultDisplayBufferCompositor. This means for the fastest refresh
     * rate of all attached outputs.
     */
    local_frameno++;
    {
        std::lock_guard<std::mutex> lock(global_frameno_lock);
        if (wrapped_greater_or_equal(local_frameno, global_frameno))
            global_frameno = local_frameno;
        else
            local_frameno = global_frameno;
    }

    static bool const bypass_env{[]
    {
        auto const env = getenv("MIR_BYPASS");
        return !env || env[0] != '0';
    }()};
    bool bypassed = false;
    bool uncomposited_buffers{false};

    if (bypass_env && display_buffer.can_bypass())
    {
        // It would be *really* nice not to lock the scene for a composite pass.
        // (C.f. lp:1234018)
        // A compositor shouldn't know anything about navigating the scene,
        // it should be passed a collection of objects to render. (And any
        // locks managed by the scene - which can just lock what is needed.)
        std::unique_lock<Scene> lock(*scene);

        mc::BypassFilter filter(display_buffer);
        mc::BypassMatch match;

        // It would be *really* nice if Scene had an iterator to simplify this
        scene->for_each_if(filter, match);

        if (filter.fullscreen_on_top())
        {
            auto bypass_buf =
                match.topmost_fullscreen()->buffer(local_frameno);

            if (bypass_buf->can_bypass())
            {
                uncomposited_buffers = match.topmost_fullscreen()->buffers_ready_for_compositor() > 1;

                lock.unlock();
                display_buffer.post_update(bypass_buf);
                bypassed = true;
                renderer->suspend();
            }
        }
    }

    if (!bypassed)
    {
        // preserves buffers used in rendering until after post_update()
        std::vector<std::shared_ptr<void>> saved_resources;
        auto save_resource = [&](std::shared_ptr<void> const& r)
        {
            saved_resources.push_back(r);
        };

        display_buffer.make_current();

        auto const& view_area = display_buffer.view_area();

        mc::OcclusionFilter occlusion_search(view_area);
        mc::OcclusionMatch occlusion_match;
        scene->reverse_for_each_if(occlusion_search, occlusion_match);

        renderer->set_rotation(display_buffer.orientation());
        renderer->begin();
        mc::RenderingOperator applicator(*renderer, save_resource, local_frameno, uncomposited_buffers);
        FilterForVisibleSceneInRegion selector(view_area, occlusion_match);
        scene->for_each_if(selector, applicator);
        renderer->end();

        display_buffer.post_update();

        // This is a frig to avoid lp:1286190
        if (size_of_last_pass)
        {
            uncomposited_buffers |= saved_resources.empty();
        }

        size_of_last_pass = saved_resources.size();
        // End of frig
    }

    report->finished_frame(bypassed, this);
    return uncomposited_buffers;
}

