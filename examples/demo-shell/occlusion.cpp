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
 *
 * based on src/server/compositor/occlusion.cpp, which was:  
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include <mir/graphics/renderable.h>
#include <mir/compositor/scene_element.h>
#include "occlusion.h"

namespace me = mir::examples;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{
bool renderable_is_occluded(
    mg::Renderable const& renderable, 
    geom::Rectangle const& area,
    std::vector<geom::Rectangle>& coverage,
    int shadow_radius,
    int titlebar_height)
{
    static const glm::mat4 identity;
    if (renderable.transformation() != identity)
        return false;  // Weirdly transformed. Assume never occluded.

    if (!renderable.visible())
        return true;  //invisible; definitely occluded.

    //account for titlebar/shadows that the demo shell adds
    auto const& undecorated_window = renderable.screen_position();
    using namespace mir::geometry;
    geom::Rectangle decorated_window{
        geom::Point{
            undecorated_window.top_left.x,
            undecorated_window.top_left.y - geom::DeltaY{titlebar_height}
        },
        geom::Size{
            undecorated_window.size.width.as_int() + shadow_radius,
            undecorated_window.size.height.as_int() + shadow_radius + titlebar_height
        }
    };

    auto const& clipped_decorated_window = decorated_window.intersection_with(area);
    
    if (clipped_decorated_window == geom::Rectangle{})
        return true;  // Not in the area; definitely occluded.

    // Not weirdly transformed but also not on this monitor? Don't care...
    if (!area.overlaps(clipped_decorated_window))
        return true;  // Not on the display; definitely occluded.

    bool occluded = false;
    for (const auto &r : coverage)
    {
        if (r.contains(clipped_decorated_window))
        {
            occluded = true;
            break;
        }
    }

    if (!occluded && renderable.alpha() == 1.0f && !renderable.shaped())
        coverage.push_back(decorated_window);

    return occluded;
}
}

mc::SceneElementSequence me::filter_occlusions_from(
    mc::SceneElementSequence& elements_list,
    geom::Rectangle const& area,
    int shadow_radius,
    int titlebar_height)
{
    mc::SceneElementSequence elements;
    std::vector<geom::Rectangle> coverage;
    auto it = elements_list.rbegin();
    while (it != elements_list.rend())
    {
        auto const renderable = (*it)->renderable();
        if (renderable_is_occluded(*renderable, area, coverage, shadow_radius, titlebar_height))
        {
            elements.insert(elements.begin(), *it);
            it = mc::SceneElementSequence::reverse_iterator(elements_list.erase(std::prev(it.base())));
        }
        else
        {
            it++;
        }
    }
    return elements;
}
