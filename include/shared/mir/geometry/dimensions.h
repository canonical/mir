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

#ifndef MIR_GEOMETRY_DIMENSIONS_H_
#define MIR_GEOMETRY_DIMENSIONS_H_

#include <cstdint>
#include <iosfwd>

namespace mir
{

/// Basic geometry types. Types for dimensions, displacements, etc.
/// and the operations that they support.
namespace geometry
{

namespace detail
{
enum DimensionTag { width, height, x, y, dx, dy, stride };

template<DimensionTag Tag>
class IntWrapper
{
public:
    typedef int ValueType;

    IntWrapper() : value(0) {}
    template<typename AnyInteger>
    explicit IntWrapper(AnyInteger value) : value(static_cast<ValueType>(value)) {}

    uint32_t as_uint32_t() const  // TODO: Deprecate this later
    {
        return (uint32_t)value;
    }
    int as_int() const
    {
        return value;
    }
    float as_float() const
    {
        return value;
    }

private:
    ValueType value;
};

template<DimensionTag Tag>
std::ostream& operator<<(std::ostream& out, IntWrapper<Tag> const& value)
{
    out << value.as_int();
    return out;
}

template<DimensionTag Tag>
inline bool operator == (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_int() == rhs.as_int();
}

template<DimensionTag Tag>
inline bool operator != (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_int() != rhs.as_int();
}

template<DimensionTag Tag>
inline bool operator <= (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_int() <= rhs.as_int();
}

template<DimensionTag Tag>
inline bool operator >= (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_int() >= rhs.as_int();
}

template<DimensionTag Tag>
inline bool operator < (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_int() < rhs.as_int();
}

template<DimensionTag Tag>
inline bool operator > (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_int() > rhs.as_int();
}
} // namespace detail

typedef detail::IntWrapper<detail::width> Width;
typedef detail::IntWrapper<detail::height> Height;
// Just to be clear, mir::geometry::Stride is the stride of the buffer in bytes
typedef detail::IntWrapper<detail::stride> Stride;

typedef detail::IntWrapper<detail::x> X;
typedef detail::IntWrapper<detail::y> Y;
typedef detail::IntWrapper<detail::dx> DeltaX;
typedef detail::IntWrapper<detail::dy> DeltaY;

// Adding deltas is fine
inline DeltaX operator+(DeltaX lhs, DeltaX rhs) { return DeltaX(lhs.as_int() + rhs.as_int()); }
inline DeltaY operator+(DeltaY lhs, DeltaY rhs) { return DeltaY(lhs.as_int() + rhs.as_int()); }
inline DeltaX operator-(DeltaX lhs, DeltaX rhs) { return DeltaX(lhs.as_int() - rhs.as_int()); }
inline DeltaY operator-(DeltaY lhs, DeltaY rhs) { return DeltaY(lhs.as_int() - rhs.as_int()); }

// Adding deltas to co-ordinates is fine
inline X operator+(X lhs, DeltaX rhs) { return X(lhs.as_int() + rhs.as_int()); }
inline Y operator+(Y lhs, DeltaY rhs) { return Y(lhs.as_int() + rhs.as_int()); }
inline X operator-(X lhs, DeltaX rhs) { return X(lhs.as_int() - rhs.as_int()); }
inline Y operator-(Y lhs, DeltaY rhs) { return Y(lhs.as_int() - rhs.as_int()); }

// Subtracting coordinates is fine
inline DeltaX operator-(X lhs, X rhs) { return DeltaX(lhs.as_int() - rhs.as_int()); }
inline DeltaY operator-(Y lhs, Y rhs) { return DeltaY(lhs.as_int() - rhs.as_int()); }

template<typename Target, typename Source>
inline Target dim_cast(Source s) { return Target(s.as_int()); }
}
}

#endif /* MIR_GEOMETRY_DIMENSIONS_H_ */
