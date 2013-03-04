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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GEOMETRY_SIZE_H_
#define MIR_GEOMETRY_SIZE_H_

#include "dimensions.h"
#include <ostream>

namespace mir
{
namespace geometry
{

struct Size
{
    Size() {}

    Size(const Width& w, const Height& h) : width {w}, height {h}
    {
    }

    Width width;
    Height height;
};

inline bool operator == (Size const& lhs, Size const& rhs)
{
    return lhs.width == rhs.width && lhs.height == rhs.height;
}

inline bool operator != (Size const& lhs, Size const& rhs)
{
    return lhs.width != rhs.width || lhs.height != rhs.height;
}

inline std::ostream& operator<<(std::ostream& out, Size const& value)
{
    out << '(' << value.width << ", " << value.height << ')';
    return out;
}

}
}

#endif /* MIR_GEOMETRY_SIZE_H_ */
