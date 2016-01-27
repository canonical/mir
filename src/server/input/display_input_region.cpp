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
#include "mir/graphics/display_configuration.h"

#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

void mi::DisplayInputRegion::set_display_configuration(mg::DisplayConfiguration const& config)
{
    std::unique_lock<std::mutex> lock(rectangle_guard);
    rectangles.clear();
    apply_config(config);
}

void mi::DisplayInputRegion::apply_config(mg::DisplayConfiguration const& config)
{
    config.for_each_output([this](mg::DisplayConfigurationOutput const& output)
                           {
                                if (output.power_mode == mir_power_mode_on &&
                                    output.current_mode_index < output.modes.size())
                                    rectangles.add(geom::Rectangle(output.top_left, output.modes[output.current_mode_index].size));
                           });
}

geom::Rectangle mi::DisplayInputRegion::bounding_rectangle()
{
    //TODO: This region is mainly used for scaling touchscreen coordinates, so the caller
    // probably wants the full list of rectangles. Additional work is needed
    // to group a touchscreen with a display. So for now, just return the view area
    // of the first display, as that matches the most common systems (laptops with touchscreens,
    // phone/tablets with touchscreens).
    if (rectangles.size() != 0)
        return *rectangles.begin();
    else
        return geom::Rectangle{};
}

void mi::DisplayInputRegion::confine(geom::Point& point)
{
    rectangles.confine(point);
}
