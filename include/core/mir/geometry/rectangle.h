/*
 * Copyright © 2012, 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GEOMETRY_RECTANGLE_H_
#define MIR_GEOMETRY_RECTANGLE_H_

#include "rectangle_generic.h"
#include "point.h"
#include "size.h"
#include "displacement.h"

namespace mir
{
namespace geometry
{

struct Rectangle : generic::Rectangle<Point, Size>
{
    using generic::Rectangle<Point, Size>::Rectangle;

    Rectangle intersection_with(Rectangle const& r) const
    {
        return intersection_of(*this, r);
    }
};
}
}

#endif /* MIR_GEOMETRY_RECTANGLE_H_ */
