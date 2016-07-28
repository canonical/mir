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


#ifndef MIR_INPUT_SEAT_REPORT_H_
#define MIR_INPUT_SEAT_REPORT_H_

#include "mir/geometry/rectangles.h"

namespace mir
{
namespace input
{
class SeatReport
{
public:
    virtual ~SeatReport() = default;
    
    virtual void seat_set_confinement_region_called(geometry::Rectangles const& regions) = 0;
};
}
}

#endif /* MIR_INPUT_SEAT_REPORT_H_ */
