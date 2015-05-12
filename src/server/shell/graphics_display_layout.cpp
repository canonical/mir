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
        auto tl_closed = rect.top_left;
        auto br_closed = rect.bottom_right() - geom::Displacement{1,1};

        geom::Rectangles rectangles;
        rectangles.add(output);

        rectangles.confine(tl_closed);
        rectangles.confine(br_closed);

        rect.top_left = tl_closed;
        rect.size = as_size(br_closed - tl_closed + geom::Displacement{1, 1});
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
            output.current_mode_index < output.modes.size())
        {
            rect.top_left = output.top_left;
            rect.size = output.extents().size;
            placed = true;
        }
    });

    if (!placed)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to place surface in requested output"));
}

geom::Rectangle msh::GraphicsDisplayLayout::get_output_for(geometry::Rectangle& rect)
{
    int max_area = -1;
    geometry::Rectangle best = rect;

    display->for_each_display_sync_group([&](mg::DisplaySyncGroup& group) 
    {
        group.for_each_display_buffer([&](mg::DisplayBuffer const& db)
            {
                auto const& screen = db.view_area();
                auto const& overlap = rect.intersection_with(screen);
                int area = overlap.size.width.as_int() *
                           overlap.size.height.as_int();

                if (area > max_area)
                {
                    best = screen;
                    max_area = area;
                }
            });
    });

    return best;
}
