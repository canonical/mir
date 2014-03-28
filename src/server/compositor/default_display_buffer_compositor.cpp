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
#include "mir/graphics/cursor.h"
#include "mir/compositor/buffer_stream.h"
#include "bypass.h"
#include "occlusion.h"
#include <mutex>
#include <cstdlib>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
using namespace mir;

namespace
{

struct FilterForUndrawnSurfaces : public mc::FilterForScene
{
    FilterForUndrawnSurfaces(
        mc::OcclusionMatch const& occlusions)
        : occlusions(occlusions)
    {
    }
    bool operator()(mg::Renderable const& r)
    {
        return !occlusions.occluded(r);
    }

    mc::OcclusionMatch const& occlusions;
};

class SoftCursor : public mg::Cursor
{
public:
    SoftCursor(mc::DefaultDisplayBufferCompositor& compositor)
        : compositor(compositor)
    {
    }

    void set_image(void const*, geometry::Size) override
    {
        // TODO: Implement software cursor image setting later
    }

    void move_to(geometry::Point position) override
    {
        compositor.on_cursor_movement(position);
    }

private:
    mc::DefaultDisplayBufferCompositor& compositor;
};

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
      soft_cursor{new SoftCursor(*this)},
      last_pass_rendered_anything{false},
      viewport(display_buffer.view_area()),
      zoom_mag{1.0f}
{
}


bool mc::DefaultDisplayBufferCompositor::composite()
{
    report->began_frame(this);

    static bool const bypass_env{[]
    {
        auto const env = getenv("MIR_BYPASS");
        return !env || env[0] != '0';
    }()};
    bool bypassed = false;
    bool uncomposited_buffers{false};
    bool should_bypass = bypass_env && viewport == display_buffer.view_area();

    if (should_bypass && display_buffer.can_bypass())
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
            /*
             * Notice the user_id we pass to buffer() here has to be
             * different to the one used in the Renderer. This is in case
             * the below if() fails we want to complete the frame using the
             * same buffer (different user_id required).
             */
            auto bypass_buf = match.topmost_fullscreen()->buffer(this);

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
        display_buffer.make_current();

        mc::OcclusionFilter occlusion_search(viewport);
        mc::OcclusionMatch occlusion_match;
        scene->reverse_for_each_if(occlusion_search, occlusion_match);

        renderer->set_viewport(viewport);
        renderer->set_rotation(display_buffer.orientation());
        renderer->begin();
        mc::RenderingOperator applicator(*renderer);
        FilterForUndrawnSurfaces selector(occlusion_match);
        scene->for_each_if(selector, applicator);
        renderer->end();

        display_buffer.post_update();

        uncomposited_buffers |= applicator.uncomposited_buffers();

        // This is a frig to avoid lp:1286190
        if (last_pass_rendered_anything && !applicator.anything_was_rendered())
        {
            uncomposited_buffers = true;
        }

        last_pass_rendered_anything = applicator.anything_was_rendered();
        // End of frig
    }

    report->finished_frame(bypassed, this);
    return uncomposited_buffers;
}

std::weak_ptr<graphics::Cursor>
mc::DefaultDisplayBufferCompositor::cursor() const
{
    return soft_cursor;
}

void mc::DefaultDisplayBufferCompositor::on_cursor_movement(
    geometry::Point const& p)
{
    cursor_pos = p;
    if (zoom_mag != 1.0f)
        update_viewport();
}

void mc::DefaultDisplayBufferCompositor::zoom(float mag)
{
    zoom_mag = mag;
    update_viewport();
}

void mc::DefaultDisplayBufferCompositor::update_viewport()
{
    auto const& view_area = display_buffer.view_area();

    if (zoom_mag == 1.0f)
    {
        // The below calculations should yield the same result as this, but
        // just in case there are any floating point precision errors,
        // set it precisely:
        viewport = view_area;
    }
    else
    {
        int db_width = view_area.size.width.as_int();
        int db_height = view_area.size.height.as_int();
        int db_x = view_area.top_left.x.as_int();
        int db_y = view_area.top_left.y.as_int();
    
        float zoom_width = db_width / zoom_mag;
        float zoom_height = db_height / zoom_mag;
    
        float screen_x = cursor_pos.x.as_int() - db_x;
        float screen_y = cursor_pos.y.as_int() - db_y;

        float normal_x = screen_x / db_width;
        float normal_y = screen_y / db_height;
    
        // Position the viewport so the cursor location matches up.
        // This assumes the hardware cursor still traverses the physical
        // screen and isn't being warped.
        int zoom_x = db_x + (db_width - zoom_width) * normal_x;
        int zoom_y = db_y + (db_height - zoom_height) * normal_y;

        viewport = {{zoom_x, zoom_y}, {zoom_width, zoom_height}};
    }
}
