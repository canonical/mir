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
#include "mir/graphics/display_buffer.h"
#include "bypass.h"

using namespace mir;
using namespace mir::compositor;

BypassFilter::BypassFilter(const graphics::DisplayBuffer &display_buffer)
        : display_buffer(display_buffer)
{
    const geometry::Rectangle &rect = display_buffer.view_area();
    int width = rect.size.width.as_int();
    int height = rect.size.height.as_int();

    /*
     * For a surface to exactly fit the display_buffer, its transformation
     * will look exactly like this:
     */
    fullscreen[0][0] = width;
    fullscreen[0][1] = 0.0f;
    fullscreen[0][2] = 0.0f;
    fullscreen[0][3] = 0.0f;

    fullscreen[1][0] = 0.0f;
    fullscreen[1][1] = height;
    fullscreen[1][2] = 0.0f;
    fullscreen[1][3] = 0.0f;

    fullscreen[2][0] = 0.0f;
    fullscreen[2][1] = 0.0f;
    fullscreen[2][2] = 0.0f;
    fullscreen[2][3] = 0.0f;

    fullscreen[3][0] = rect.top_left.x.as_int() + width / 2;
    fullscreen[3][1] = rect.top_left.y.as_int() + height / 2;
    fullscreen[3][2] = 0.0f;
    fullscreen[3][3] = 1.0f;
}

bool BypassFilter::operator()(const CompositingCriteria &criteria)
{
    if (criteria.alpha() != 1.0f || criteria.shaped())
        return false;

    if (!all_orthogonal)
        return false;

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

    // Any weird transformations? Then we can't risk any bypass
    if (!orthogonal)
    {
        all_orthogonal = false;
        return false;
    }

    // Not weirdly transformed but also not on this monitor? Don't care...
    // This will also check the surface is not hidden and has been posted.
    if (!criteria.should_be_rendered_in(display_buffer.view_area()))
        return false;

    // Transformed perfectly to fit the monitor? Bypass!
    topmost_fits = criteria.transformation() == fullscreen;
    return topmost_fits;
}

bool BypassFilter::fullscreen_on_top() const
{
    return all_orthogonal && topmost_fits;
}

void BypassMatch::operator()(const CompositingCriteria &,
                             compositor::BufferStream &stream)
{
    latest = &stream;
}

compositor::BufferStream *BypassMatch::topmost_fullscreen() const
{
    return latest;
}
