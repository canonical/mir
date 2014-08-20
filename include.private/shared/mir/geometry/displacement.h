/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GEOMETRY_DISPLACEMENT_H_
#define MIR_GEOMETRY_DISPLACEMENT_H_

#include "dimensions.h"
#include "point.h"

#include <iosfwd>

namespace mir
{
namespace geometry
{

struct Displacement
{
    Displacement() {}
    Displacement(Displacement const&) = default;
    Displacement& operator=(Displacement const&) = default;

    template<typename DeltaXType, typename DeltaYType>
    Displacement(DeltaXType&& dx, DeltaYType&& dy) : dx{dx}, dy{dy} {}

    float length_squared() const
    {
        return dx.as_float() * dx.as_float() + dy.as_float() * dy.as_float();
    }

    DeltaX dx;
    DeltaY dy;
};

inline bool operator==(Displacement const& lhs, Displacement const& rhs)
{
    return lhs.dx == rhs.dx && lhs.dy == rhs.dy;
}

inline bool operator!=(Displacement const& lhs, Displacement const& rhs)
{
    return lhs.dx != rhs.dx || lhs.dy != rhs.dy;
}

std::ostream& operator<<(std::ostream& out, Displacement const& value);

inline Displacement operator+(Displacement const& lhs, Displacement const& rhs)
{
    return Displacement{lhs.dx + rhs.dx, lhs.dy + rhs.dy};
}

inline Displacement operator-(Displacement const& lhs, Displacement const& rhs)
{
    return Displacement{lhs.dx - rhs.dx, lhs.dy - rhs.dy};
}

inline Point operator+(Point const& lhs, Displacement const& rhs)
{
    return Point{lhs.x + rhs.dx, lhs.y + rhs.dy};
}

inline Point operator-(Point const& lhs, Displacement const& rhs)
{
    return Point{lhs.x - rhs.dx, lhs.y - rhs.dy};
}

inline Displacement operator-(Point const& lhs, Point const& rhs)
{
    return Displacement{lhs.x - rhs.x, lhs.y - rhs.y};
}

inline bool operator<(Displacement const& lhs, Displacement const& rhs)
{
    return lhs.length_squared() < rhs.length_squared();
}

}
}

#endif /* MIR_GEOMETRY_DISPLACEMENT_H_ */
