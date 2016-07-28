/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#include "seat_report.h"

#include "mir/logging/logger.h"
namespace ml = mir::logging;
namespace mrl = mir::report::logging;

namespace
{
char const* const component = "input::Seat";
}

mrl::SeatReport::SeatReport(std::shared_ptr<ml::Logger> const& log) :
    log(log)
{
}

void mrl::SeatReport::seat_set_confinement_region_called(geometry::Rectangles const& regions)
{
    auto bound_rect = regions.bounding_rectangle();
    log->log(ml::Severity::informational, "set_confinement_region(\"" +
                                          std::to_string(bound_rect.top_left.x.as_int()) + " " +
                                          std::to_string(bound_rect.top_left.y.as_int()) + " " +
                                          std::to_string(bound_rect.size.width.as_int()) + " " +
                                          std::to_string(bound_rect.size.height.as_int()) + "\")", component);
}
