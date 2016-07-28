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

#include "mir/report/lttng/mir_tracepoint.h"

#include "seat_report.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "seat_report_tp.h"

namespace geom = mir::geometry;

void mir::report::lttng::SeatReport::seat_set_confinement_region_called(geom::Rectangles const& regions)
{
    auto bound_rect = regions.bounding_rectangle();
    mir_tracepoint(mir_server_seat, seat_set_confinement_region_called,
        bound_rect.top_left.x.as_int(),
        bound_rect.top_left.y.as_int(),
        bound_rect.size.width.as_int(),
        bound_rect.size.height.as_int());
}
