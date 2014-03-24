/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/geometry/rectangle.h"
#include "mir/graphics/renderable.h"
#include "occlusion.h"

namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace mg=mir::graphics;

mc::OcclusionFilter::OcclusionFilter(const geometry::Rectangle &area)
    : area(area)
{
}
namespace
{
bool filter(
    mg::Renderable const& renderable, 
    geom::Rectangle const& area,
    std::vector<geom::Rectangle>& coverage)
{
    static const glm::mat4 identity;
    if (renderable.transformation() != identity)
        return false;  // Weirdly transformed. Assume never occluded.

    if (!renderable.should_be_rendered_in(area))
        return true;  // Not on the display, or invisible; definitely occluded.

    bool occluded = false;
    geom::Rectangle const& window = renderable.screen_position();
    for (const auto &r : coverage)
    {
        if (r.contains(window))
        {
            occluded = true;
            break;
        }
    }

    if (!occluded && renderable.alpha() == 1.0f && !renderable.shaped())
        coverage.push_back(window);

    return occluded;
}
}

bool mc::OcclusionFilter::operator()(mg::Renderable const& renderable)
{
    return filter(renderable, area, coverage);
}

void mc::filter_occlusions_from(
    mg::RenderableList& list,
    geom::Rectangle const& area)
{
    std::vector<geom::Rectangle> coverage;

    auto it = list.begin();
    while (it != list.end())
    {
        if (filter(**it, area, coverage))
        {
            it = list.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void mc::OcclusionMatch::operator()(mg::Renderable const& renderable)
{
    hidden.insert(&renderable);
}

bool mc::OcclusionMatch::occluded(mg::Renderable const& renderable) const
{
    return hidden.find(&renderable) != hidden.end();
}
