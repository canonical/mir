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

#include "mir/shell/display_surface_boundaries.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"

#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"

namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

#include <memory>

msh::DisplaySurfaceBoundaries::DisplaySurfaceBoundaries(
    std::shared_ptr<mg::Display> const& display)
    : display{display}
{
}

void msh::DisplaySurfaceBoundaries::clip_to_screen(geometry::Rectangle& rect)
{
    auto screen = get_screen_for(rect);

    if (screen.size.width != geom::Width{0} && screen.size.height != geom::Height{0} &&
        rect.size.width != geom::Width{0} && rect.size.height != geom::Height{0})
    {
        auto tl = rect.top_left;
        auto br = rect.bottom_right();

        geom::Rectangles rectangles;
        rectangles.add(screen);

        rectangles.confine_point(br);

        rect.size = 
            geom::Size{br.x.as_int() - tl.x.as_int() + 1,
                       br.y.as_int() - tl.y.as_int() + 1};
    }
    else
    {
        rect.size = geom::Size{0,0};
    }
}

void msh::DisplaySurfaceBoundaries::make_fullscreen(geometry::Rectangle& rect)
{
    auto screen = get_screen_for(rect);
    rect = screen;
}

geom::Rectangle msh::DisplaySurfaceBoundaries::get_screen_for(geometry::Rectangle& rect)
{
    geom::Rectangle screen;

    /*
     * TODO: We need a better heuristic to decide on which screen a
     * rectangle/surface belongs too
     */
    display->for_each_display_buffer(
        [&screen,&rect](mg::DisplayBuffer const& db)
        {
            auto view_area = db.view_area();
            if (view_area.contains(rect.top_left))
                screen = view_area;
        });

    return screen;
}
