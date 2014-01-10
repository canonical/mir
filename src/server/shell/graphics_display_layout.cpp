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

#include "graphics_display_layout.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"

#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"
#include "mir/geometry/displacement.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

msh::GraphicsDisplayLayout::GraphicsDisplayLayout(
    std::shared_ptr<mg::Display> const& display)
    : display{display}
{
}

void msh::GraphicsDisplayLayout::clip_to_output(geometry::Rectangle& rect)
{
    auto output = get_output_for(rect);

    if (output.size.width > geom::Width{0} && output.size.height > geom::Height{0} &&
        rect.size.width > geom::Width{0} && rect.size.height > geom::Height{0})
    {
        auto tl = rect.top_left;
        auto br_closed = rect.bottom_right() - geom::Displacement{1,1};

        geom::Rectangles rectangles;
        rectangles.add(output);

        rectangles.confine(br_closed);

        rect.size =
            geom::Size{br_closed.x.as_int() - tl.x.as_int() + 1,
                       br_closed.y.as_int() - tl.y.as_int() + 1};
    }
    else
    {
        rect.size = geom::Size{0,0};
    }
}

void msh::GraphicsDisplayLayout::size_to_output(geometry::Rectangle& rect)
{
    auto output = get_output_for(rect);
    rect = output;
}

void msh::GraphicsDisplayLayout::place_in_output(
    graphics::DisplayConfigurationOutputId id,
    geometry::Rectangle& rect)
{
    auto config = display->configuration();
    bool placed = false;

    /* Accept only fullscreen placements for now */
    config->for_each_output([&](mg::DisplayConfigurationOutput const& output)
    {
        if (output.id == id &&
            output.current_mode_index < output.modes.size() &&
            rect.size == output.extents().size)
        {
            rect.top_left = output.top_left;
            placed = true;
        }
    });

    if (!placed)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to place surface in requested output"));
}

geom::Rectangle msh::GraphicsDisplayLayout::get_output_for(geometry::Rectangle& rect)
{
    geom::Rectangle output;

    /*
     * TODO: We need a better heuristic to decide in which output a
     * rectangle/surface belongs.
     */
    display->for_each_display_buffer(
        [&output,&rect](mg::DisplayBuffer const& db)
        {
            auto view_area = db.view_area();
            if (view_area.contains(rect.top_left))
                output = view_area;
        });

    return output;
}
