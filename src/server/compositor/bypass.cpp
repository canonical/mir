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

#include "mir/graphics/renderable.h"
#include "mir/graphics/display_buffer.h"
#include "bypass.h"

using namespace mir;
using namespace mir::compositor;
using namespace mir::graphics;

BypassFilter::BypassFilter(const graphics::DisplayBuffer &display_buffer)
        : display_buffer(display_buffer)
{
}

bool BypassFilter::operator()(const Renderable &renderable)
{
    if (!all_orthogonal)
        return false;

    // Any weird transformations? Then we can't risk any bypass
    static const glm::mat4 identity;
    if (renderable.transformation() != identity)
    {
        all_orthogonal = false;
        return false;
    }

    auto const& view_area = display_buffer.view_area();

    // Not weirdly transformed but also not on this monitor? Don't care...
    // This will also check the surface is not hidden and has been posted.
    if (!renderable.should_be_rendered_in(view_area))
        return false;

    topmost_fits = false;

    if (renderable.alpha() != 1.0f || renderable.shaped())
        return false;

    // Transformed perfectly to fit the monitor? Bypass!
    topmost_fits = renderable.screen_position() == view_area;
    return topmost_fits;
}

bool BypassFilter::fullscreen_on_top() const
{
    return all_orthogonal && topmost_fits;
}

void BypassMatch::operator()(const Renderable &r)
{
    latest = &r;
}

const Renderable *BypassMatch::topmost_fullscreen() const
{
    return latest;
}
