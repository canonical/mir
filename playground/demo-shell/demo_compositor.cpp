/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/display_buffer.h"
#include "mir/compositor/compositor_report.h"
#include "mir/compositor/scene_element.h"
#include "demo_compositor.h"

namespace me = mir::examples;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

std::mutex                              me::DemoCompositor::instances_mutex;
std::unordered_set<me::DemoCompositor*> me::DemoCompositor::instances;

me::DemoCompositor::DemoCompositor(
    mg::DisplayBuffer& display_buffer,
    std::shared_ptr<mc::CompositorReport> const& report) :
    display_buffer(display_buffer),
    report(report),
    viewport(display_buffer.view_area()),
    zoom_mag{1.0f},
    renderer(
        display_buffer.view_area(),
        30.0f, //titlebar_height
        80.0f) //shadow_radius
{
    std::lock_guard<std::mutex> lock(instances_mutex);
    instances.insert(this);
}

me::DemoCompositor::~DemoCompositor()
{
    std::lock_guard<std::mutex> lock(instances_mutex);
    instances.erase(this);
}

void me::DemoCompositor::for_each(std::function<void(DemoCompositor&)> f)
{
    std::lock_guard<std::mutex> lock(instances_mutex);
    for (auto& i : instances)
        f(*i);
}

void me::DemoCompositor::composite(mc::SceneElementSequence&& elements)
{
    report->began_frame(this);
    //a simple filtering out of renderables that shouldn't be drawn
    //the elements should be notified if they are rendered or not
    bool nonrenderlist_elements{false};
    mg::RenderableList renderable_list;
    DecorMap decorated;

    for(auto const& it : elements)
    {
        auto const& renderable = it->renderable();
        
        bool embellished = false;
        if (auto decor = it->decoration())
        {
            embellished = decor->type != mc::Decoration::Type::none;
            decorated[renderable->id()] = std::move(decor);
        }

        if (embellished || viewport.overlaps(renderable->screen_position()))
        {
            renderable_list.push_back(renderable);

            /*
             * TODO: This logic could be replaced more cleanly in future by
             *       the surface stack logic setting decoration status more
             *       accurately for fullscreen surfaces.
             */
            // Fullscreen and opaque? Definitely no embellishment
            if (renderable->screen_position() == viewport &&
                renderable->alpha() == 1.0f &&
                !renderable->shaped() &&
                renderable->transformation() == glm::mat4())
            {
                embellished = false;
                nonrenderlist_elements = false; // Don't care what's underneath
            }

            it->rendered();
        }
        else
        {
            it->occluded();
        }
        nonrenderlist_elements |= embellished;
    }

    /*
     * Note: Buffer lifetimes are ensured by the two objects holding
     *       references to them; elements and renderable_list.
     *       So no buffer is going to be released back to the client till
     *       both of those containers get destroyed (end of the function).
     *       Actually, there's a third reference held by the texture cache
     *       in GLRenderer, but that gets released earlier in render().
     */
    elements.clear();  // Release those that didn't make it to renderable_list

    if (!nonrenderlist_elements &&
        viewport == display_buffer.view_area() &&  // no bypass while zoomed
        display_buffer.post_renderables_if_optimizable(renderable_list))
    {
        report->renderables_in_frame(this, renderable_list);
        renderer.suspend();
    }
    else
    {
        display_buffer.make_current();

        renderer.set_rotation(display_buffer.orientation());
        renderer.set_viewport(viewport);
        renderer.begin(std::move(decorated));
        renderer.render(renderable_list);

        display_buffer.gl_swap_buffers();
        report->renderables_in_frame(this, renderable_list);
        report->rendered_frame(this);

        // Release buffers back to the clients now that the swap has returned.
        // It's important to do this before starting on the potentially slow
        // flip() ...
        // FIXME: This clear() call is blocking a little (LP: #1395421)
        renderable_list.clear();
    }

    report->finished_frame(this);
}

void me::DemoCompositor::on_cursor_movement(
    geometry::Point const& p)
{
    cursor_pos = p;
    if (zoom_mag != 1.0f)
        update_viewport();
}

void me::DemoCompositor::zoom(float mag)
{
    zoom_mag = mag;
    update_viewport();
}

void me::DemoCompositor::set_colour_effect(me::ColourEffect e)
{
    renderer.set_colour_effect(e);
}

void me::DemoCompositor::update_viewport()
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
    
        // Note the 0.5f. This is because cursors (and all input in general)
        // measures coordinates at the centre of a pixel. But GL measures to
        // the top-left corner of a pixel.
        float screen_x = cursor_pos.x.as_float() + 0.5f - db_x;
        float screen_y = cursor_pos.y.as_float() + 0.5f - db_y;

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
