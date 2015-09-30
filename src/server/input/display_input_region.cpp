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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display_input_region.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"

#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mi::DisplayInputRegion::DisplayInputRegion(
    std::shared_ptr<mg::Display> const& display)
    : display{display}
{
}

geom::Rectangle mi::DisplayInputRegion::bounding_rectangle()
{
    geom::Rectangles rectangles;

    display->for_each_display_sync_group([&rectangles](mg::DisplaySyncGroup& group)
    {
        group.for_each_display_buffer(
            [&rectangles](mg::DisplayBuffer const& buffer)
            {
                rectangles.add(buffer.view_area());
            });
    });

    //TODO: This region is mainly used for scaling touchscreen coordinates, so the caller
    // probably wants the full list of rectangles. Additional work is needed
    // to group a touchscreen with a display. So for now, just return the view area
    // of the first display, as that matches the most common systems (laptops with touchscreens,
    // phone/tablets with touchscreens).
    return *rectangles.begin();
}

void mi::DisplayInputRegion::confine(geom::Point& point)
{
    geom::Rectangles rectangles;

    display->for_each_display_sync_group([&rectangles](mg::DisplaySyncGroup& group)
    {
        group.for_each_display_buffer(
            [&rectangles](mg::DisplayBuffer const& buffer)
            {
                rectangles.add(buffer.view_area());
            });
    });

    rectangles.confine(point);
}
