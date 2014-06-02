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
#include "demo_compositor.h"

namespace me = mir::examples;
namespace mg = mir::graphics;
namespace mc = mir::compositor;

me::DemoCompositor::DemoCompositor(
    mg::DisplayBuffer& display_buffer,
    std::shared_ptr<mc::Scene> const& scene,
    mg::GLProgramFactory const& factory,
    std::shared_ptr<mc::CompositorReport> const& report) :
    display_buffer(display_buffer),
    scene(scene),
    report(report),
    renderer(factory, display_buffer.view_area()) 
{
}

bool me::DemoCompositor::composite()
{
#if 0
    if (display_buffer.can_bypass())
    {
        mc::BypassMatch bypass_match(view_area);
        auto bypass_it = std::find_if(renderable_list.rbegin(), renderable_list.rend(), bypass_match);
        if (bypass_it != renderable_list.rend())
        {
            auto bypass_buf = (*bypass_it)->buffer();
            if (bypass_buf->can_bypass())
            {
                display_buffer.post_update(bypass_buf);
                bypassed = true;
                renderer->suspend();
            }
        }
    }
    if (!bypassed)
    {
#endif
    report->began_frame(this);

    auto renderable_list = scene->renderable_list_for(this);
    //mc::filter_occlusions_from(renderable_list, display_buffer.view_area();

    display_buffer.make_current();

    renderer.set_rotation(display_buffer.orientation());
    renderer.begin();
    renderer.render(renderable_list);
    display_buffer.post_update();
    renderer.end();
    report->finished_frame(false, this);

    return false;
}
