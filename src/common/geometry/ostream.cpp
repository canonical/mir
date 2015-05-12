/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 */

#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "mir/geometry/rectangle.h"
#include "mir/geometry/rectangles.h"

#include <ostream>

namespace geom = mir::geometry;

std::ostream& geom::operator<<(std::ostream& out, Displacement const& value)
{
    out << '(' << value.dx << ", " << value.dy << ')';
    return out;
}

std::ostream& geom::operator<<(std::ostream& out, Point const& value)
{
    out << '(' << value.x << ", " << value.y << ')';
    return out;
}

std::ostream& geom::operator<<(std::ostream& out, Size const& value)
{
    out << '(' << value.width << ", " << value.height << ')';
    return out;
}

std::ostream& geom::operator<<(std::ostream& out, Rectangle const& value)
{
    out << '(' << value.top_left << ", " << value.size << ')';
    return out;
}

std::ostream& geom::operator<<(std::ostream& out, Rectangles const& value)
{
    out << '[';
    for (auto const& rect : value)
        out << rect << ", ";
    out << ']';
    return out;
}
