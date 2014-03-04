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

#include "mir/compositor/compositing_criteria.h"
#include "mir/geometry/rectangle.h"
#include "occlusion.h"

using namespace mir;
using namespace mir::compositor;

OcclusionFilter::OcclusionFilter(const geometry::Rectangle &area)
        : area(area)
{
}

bool OcclusionFilter::operator()(const CompositingCriteria &criteria)
{
    const glm::mat4 &trans = criteria.transformation();
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

    // This could be replaced by adding a "CompositingCriteria::rect()"
    int width = trans[0][0];
    int height = trans[1][1];
    int x = trans[3][0] - width / 2;
    int y = trans[3][1] - height / 2;
    geometry::Rectangle window{{x, y}, {width, height}};

    if (!criteria.should_be_rendered_in(area))
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

    if (!occluded && criteria.alpha() == 1.0f && !criteria.shaped())
        coverage.push_back(window);

    return occluded;
}

void OcclusionMatch::operator()(const CompositingCriteria &criteria,
                                compositor::BufferStream &)
{
    hidden.insert(&criteria);
}

bool OcclusionMatch::occluded(const CompositingCriteria &criteria) const
{
    return hidden.find(&criteria) != hidden.end();
}
