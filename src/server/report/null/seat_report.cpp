/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

namespace mrn  = mir::report::null;
namespace geom = mir::geometry;

void mrn::SeatReport::seat_add_device(uint64_t /*id*/)
{
}

void mrn::SeatReport::seat_remove_device(uint64_t /*id*/)
{
}

void mrn::SeatReport::seat_dispatch_event(std::shared_ptr<MirEvent const> const& /*event*/)
{
}

void mrn::SeatReport::seat_set_key_state(uint64_t /*id*/, std::vector<uint32_t> const& /*scan_codes*/)
{
}

void mrn::SeatReport::seat_set_pointer_state(uint64_t /*id*/, unsigned /*buttons*/)
{
}

void mrn::SeatReport::seat_set_cursor_position(float /*cursor_x*/, float /*cursor_y*/)
{
}

void mrn::SeatReport::seat_set_confinement_region_called(geom::Rectangles const& /*regions*/)
{
}

void mrn::SeatReport::seat_reset_confinement_regions()
{
}
