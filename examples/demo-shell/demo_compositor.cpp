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
#include "mir/compositor/scene.h"
#include "mir/compositor/scene_element.h"
#include "mir/compositor/destination_alpha.h"
#include "demo_compositor.h"
#include "occlusion.h"

namespace me = mir::examples;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{
mc::DestinationAlpha destination_alpha(mg::DisplayBuffer const& db)
{
    return db.uses_alpha() ? mc::DestinationAlpha::generate_from_source : mc::DestinationAlpha::opaque;
}
}

me::DemoCompositor::DemoCompositor(
    mg::DisplayBuffer& display_buffer,
    std::shared_ptr<mc::Scene> const& scene,
    mg::GLProgramFactory const& factory,
    std::shared_ptr<mc::CompositorReport> const& report) :
    display_buffer(display_buffer),
    scene(scene),
    report(report),
    shadow_radius(80),
    titlebar_height(30),
    renderer(
        factory,
        display_buffer.view_area(),
        destination_alpha(display_buffer),
        static_cast<float>(shadow_radius),
        static_cast<float>(titlebar_height)) 
{
}

mg::RenderableList me::DemoCompositor::generate_renderables()
{
    mg::RenderableList renderable_list;
    auto elements = scene->scene_elements_for(this);
    auto occluded = me::filter_occlusions_from(elements, display_buffer.view_area(), shadow_radius, titlebar_height);
    for(auto const& it : elements)
    {
        renderable_list.push_back(it->renderable());
        it->rendered_in(this);
    }
    for(auto const& it : occluded)
        it->occluded_in(this);

    
    return renderable_list;
}

void me::DemoCompositor::composite()
{
    report->began_frame(this);

    auto renderable_list = generate_renderables();
    if (display_buffer.post_renderables_if_optimizable(renderable_list))
    {
        renderer.suspend();
        report->finished_frame(true, this);
    }
    else
    {
        display_buffer.make_current();

        renderer.set_rotation(display_buffer.orientation());
        renderer.begin();
        renderer.render(renderable_list);
        display_buffer.post_update();
        renderer.end();
        report->finished_frame(false, this);
    }
}
