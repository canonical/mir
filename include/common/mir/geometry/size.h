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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GEOMETRY_SIZE_H_
#define MIR_GEOMETRY_SIZE_H_

#include "mir/geometry/dimensions.h"
#include <iosfwd>

namespace mir
{
namespace geometry
{

struct Size
{
    Size() {}
    Size(Size const&) = default;
    Size& operator=(Size const&) = default;

    template<typename WidthType, typename HeightType>
    Size(WidthType&& width, HeightType&& height) : width(width), height(height) {}

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

std::ostream& operator<<(std::ostream& out, Size const& value);
}
}

#endif /* MIR_GEOMETRY_SIZE_H_ */
