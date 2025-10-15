/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/geometry/rectangle.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "occlusion.h"

#include <vector>

using namespace mir::geometry;
using namespace mir::graphics;
using namespace mir::compositor;

namespace
{
bool renderable_is_occluded(
    Renderable const& renderable,
    Rectangle const& area,
    std::vector<Rectangle>& coverage)
{
    static glm::mat4 const identity(1);
    static Rectangle const empty{};

    if (renderable.transformation() != identity)
        return false;  // Weirdly transformed. Assume never occluded.

    auto const& window = renderable.screen_position();
    auto const& clipped_window = intersection_of(window, area);

    if (clipped_window == empty)
        return true;  // Not in the area; definitely occluded.

    bool occluded = false;
    for (auto const& r : coverage)
    {
        if (r.contains(clipped_window))
        {
            occluded = true;
            break;
        }
    }

    if (!occluded)
    {
        if (renderable.shaped())
        {
            if (auto const opaque_region = renderable.opaque_region())
            {
                for (auto const& subregion : *opaque_region)
                {
                    auto const clipped_subregion = intersection_of(subregion, area);
                    coverage.push_back(clipped_subregion);
                }
            }
            // Client didn't send an opaque region
        }
        else if (renderable.alpha() == 1.0f)
        {
            coverage.push_back(clipped_window);
        }
    }
    return occluded;
}
}

std::pair<OccludedElementSequence, SceneElementSequence> mir::compositor::split_occluded_and_visible(
    SceneElementSequence&& elements, Rectangle const& area)
{
    std::pair<OccludedElementSequence, SceneElementSequence> result{{}, std::move(elements)};
    std::vector<Rectangle> coverage;

    auto& [occluded, visible] = result;

    auto it = visible.rbegin();
    while (it != visible.rend())
    {
        auto const renderable = (*it)->renderable();
        if (renderable_is_occluded(*renderable, area, coverage))
        {
            occluded.insert(occluded.begin(), *it);
            it = SceneElementSequence::reverse_iterator(visible.erase(std::prev(it.base())));
        }
        else
        {
            it++;
        }
    }

    return result;
}
