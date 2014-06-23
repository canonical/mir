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
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "occlusion.h"

#include <vector>

using namespace mir::geometry;
using namespace mir::graphics;

namespace
{
bool renderable_is_occluded(
    Renderable const& renderable, 
    Rectangle const& area,
    std::vector<Rectangle>& coverage)
{
    static const glm::mat4 identity;
    if (renderable.transformation() != identity)
        return false;  // Weirdly transformed. Assume never occluded.

    //TODO: remove this check, why are we getting a non visible renderable 
    //      in the list of surfaces?
    // This will check the surface is not hidden and has been posted.
    if (!renderable.visible())
        return true;  //invisible; definitely occluded.

    // Not weirdly transformed but also not on this monitor? Don't care...
    if (!area.overlaps(renderable.screen_position()))
        return true;  // Not on the display; definitely occluded.

    bool occluded = false;
    Rectangle const& window = renderable.screen_position();
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

void mir::compositor::filter_occlusions_from(
    SceneElementSequence& elements,
    Rectangle const& area)
{
    std::vector<Rectangle> coverage;
    auto it = elements.rbegin();
    while (it != elements.rend())
    {
        auto const renderable = (*it)->renderable();
        if (renderable_is_occluded(*renderable, area, coverage))
            it = SceneElementSequence::reverse_iterator(elements.erase(std::prev(it.base())));
        else
            it++;
    }
}
