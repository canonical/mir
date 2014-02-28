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

using namespace mir;
using namespace mir::compositor;
using namespace mir::graphics;

OcclusionFilter::OcclusionFilter(const geometry::Rectangle &area)
        : area(area)
{
}

bool OcclusionFilter::operator()(const Renderable &renderable)
{
    const glm::mat4 &trans = renderable.transformation();
    bool orthogonal =
        trans[0][1] == 0.0f &&
        trans[0][2] == 0.0f &&
        trans[0][3] == 0.0f &&
        trans[1][0] == 0.0f &&
        trans[1][2] == 0.0f &&
        trans[1][3] == 0.0f &&
        trans[2][0] == 0.0f &&
        trans[2][1] == 0.0f &&
        trans[2][2] == 0.0f &&
        trans[2][3] == 0.0f &&
        trans[3][2] == 0.0f &&
        trans[3][3] == 1.0f;

    if (!orthogonal)
        return false;  // Weirdly transformed. Assume never occluded.

    // This could be replaced by adding a "Renderable::rect()"
    int width = trans[0][0];
    int height = trans[1][1];
    int x = trans[3][0] - width / 2;
    int y = trans[3][1] - height / 2;
    geometry::Rectangle window{{x, y}, {width, height}};

    if (!renderable.should_be_rendered_in(area))
        return true;  // Not on the display, or invisible; definitely occluded.

    bool occluded = false;
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

void OcclusionMatch::operator()(const Renderable &renderable)
{
    hidden.insert(&renderable);
}

bool OcclusionMatch::occluded(const Renderable &renderable) const
{
    return hidden.find(&renderable) != hidden.end();
}
