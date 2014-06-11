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

bool me::DemoCompositor::composite()
{
    report->began_frame(this);

    auto renderable_list = scene->renderable_list_for(this);
    mc::filter_occlusions_from(renderable_list, display_buffer.view_area(),
        [this](mg::Renderable const& renderable) -> geom::Rectangle
        {
            //accommodate for shadows and titlebar in occlusion filtering
            using namespace mir::geometry;
            auto const& rect = renderable.screen_position();
            return geom::Rectangle{
                geom::Point{
                    rect.top_left.x,
                    rect.top_left.y - geom::DeltaY{titlebar_height}
                },
                geom::Size{
                    rect.size.width.as_int() + shadow_radius,
                    rect.size.height.as_int() + shadow_radius + titlebar_height
                }
            };
        });

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

    return false;
}
