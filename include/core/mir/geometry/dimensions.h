/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GEOMETRY_DIMENSIONS_H_
#define MIR_GEOMETRY_DIMENSIONS_H_

#include "forward.h"

#include <iosfwd>
#include <type_traits>
#include <cstdint>

namespace mir
{
namespace geometry
{
namespace generic
{
template<typename T, typename Tag>
/// Wraps a geometry value and prevents it from being accidentally used for invalid operations (such as setting a
/// width to a height or adding two x positions together). Of course, explicit casts are possible to get around
/// these restrictions (see the as_*() functions).
struct Value
{
    using ValueType = T;
    using TagType = Tag;

    template <typename Q = T>
    constexpr typename std::enable_if<std::is_integral<Q>::value, int>::type as_int() const
    {
        return this->value;
    }

    template <typename Q = T>
    constexpr typename std::enable_if<std::is_integral<Q>::value, uint32_t>::type as_uint32_t() const
    {
        return this->value;
    }

    constexpr T as_value() const noexcept
    {
        return value;
    }

    constexpr Value() noexcept : value{} {}

    Value& operator=(Value const& that) noexcept
    {
        value = that.value;
        return *this;
    }

    constexpr Value(Value const& that) noexcept
        : value{that.value}
    {
    }

    template<typename U>
    explicit constexpr Value(Value<U, Tag> const& value) noexcept
        : value{static_cast<T>(value.as_value())}
    {
    }

    template<typename U, typename std::enable_if<std::is_scalar<U>::value, bool>::type = true>
    explicit constexpr Value(U const& value) noexcept
        : value{static_cast<T>(value)}
    {
    }

    inline constexpr auto operator == (Value<T, Tag> const& rhs) const -> bool
    {
        return value == rhs.as_value();
    }

    inline constexpr auto operator != (Value<T, Tag> const& rhs) const -> bool
    {
        return value != rhs.as_value();
    }

    inline constexpr auto operator <= (Value<T, Tag> const& rhs) const -> bool
    {
        return value <= rhs.as_value();
    }

    inline constexpr auto operator >= (Value<T, Tag> const& rhs) const -> bool
    {
        return value >= rhs.as_value();
    }

    inline constexpr auto operator < (Value<T, Tag> const& rhs) const -> bool
    {
        return value < rhs.as_value();
    }

    inline constexpr auto operator > (Value<T, Tag> const& rhs) const -> bool
    {
        return value > rhs.as_value();
    }

protected:
    T value;
};

template<typename T, typename Tag>
std::ostream& operator<<(std::ostream& out, Value<T, Tag> const& value)
{
    out << value.as_value();
    return out;
}

// Adding deltas is fine
template<typename T>
inline constexpr DeltaX<T> operator+(DeltaX<T> lhs, DeltaX<T> rhs){ return DeltaX<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr DeltaY<T> operator+(DeltaY<T> lhs, DeltaY<T> rhs) { return DeltaY<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr DeltaX<T> operator-(DeltaX<T> lhs, DeltaX<T> rhs) { return DeltaX<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr DeltaY<T> operator-(DeltaY<T> lhs, DeltaY<T> rhs) { return DeltaY<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr DeltaX<T> operator-(DeltaX<T> rhs) { return DeltaX<T>(-rhs.as_value()); }
template<typename T>
inline constexpr DeltaY<T> operator-(DeltaY<T> rhs) { return DeltaY<T>(-rhs.as_value()); }
template<typename T>
inline DeltaX<T>& operator+=(DeltaX<T>& lhs, DeltaX<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline DeltaY<T>& operator+=(DeltaY<T>& lhs, DeltaY<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline DeltaX<T>& operator-=(DeltaX<T>& lhs, DeltaX<T> rhs) { return lhs = lhs - rhs; }
template<typename T>
inline DeltaY<T>& operator-=(DeltaY<T>& lhs, DeltaY<T> rhs) { return lhs = lhs - rhs; }

// Adding deltas to co-ordinates is fine
template<typename T>
inline constexpr X<T> operator+(X<T> lhs, DeltaX<T> rhs) { return X<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr Y<T> operator+(Y<T> lhs, DeltaY<T> rhs) { return Y<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr X<T> operator-(X<T> lhs, DeltaX<T> rhs) { return X<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr Y<T> operator-(Y<T> lhs, DeltaY<T> rhs) { return Y<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline X<T>& operator+=(X<T>& lhs, DeltaX<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline Y<T>& operator+=(Y<T>& lhs, DeltaY<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline X<T>& operator-=(X<T>& lhs, DeltaX<T> rhs) { return lhs = lhs - rhs; }
template<typename T>
inline Y<T>& operator-=(Y<T>& lhs, DeltaY<T> rhs) { return lhs = lhs - rhs; }

// Adding deltas to generic::Width and generic::Height is fine
template<typename T>
inline constexpr Width<T> operator+(Width<T> lhs, DeltaX<T> rhs) { return Width<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr Height<T> operator+(Height<T> lhs, DeltaY<T> rhs) { return Height<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr Width<T> operator-(Width<T> lhs, DeltaX<T> rhs) { return Width<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr Height<T> operator-(Height<T> lhs, DeltaY<T> rhs) { return Height<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline Width<T>& operator+=(Width<T>& lhs, DeltaX<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline Height<T>& operator+=(Height<T>& lhs, DeltaY<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline Width<T>& operator-=(Width<T>& lhs, DeltaX<T> rhs) { return lhs = lhs - rhs; }
template<typename T>
inline Height<T>& operator-=(Height<T>& lhs, DeltaY<T> rhs) { return lhs = lhs - rhs; }

// Adding Widths and Heights is fine
template<typename T>
inline constexpr Width<T> operator+(Width<T> lhs, Width<T> rhs) { return Width<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr Height<T> operator+(Height<T> lhs, Height<T> rhs) { return Height<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline Width<T>& operator+=(Width<T>& lhs, Width<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline Height<T>& operator+=(Height<T>& lhs, Height<T> rhs) { return lhs = lhs + rhs; }

// Subtracting coordinates is fine
template<typename T>
inline constexpr DeltaX<T> operator-(X<T> lhs, X<T> rhs) { return DeltaX<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr DeltaY<T> operator-(Y<T> lhs, Y<T> rhs) { return DeltaY<T>(lhs.as_value() - rhs.as_value()); }

//Subtracting Width<T> and Height<T> is fine
template<typename T>
inline constexpr DeltaX<T> operator-(Width<T> lhs, Width<T> rhs) { return DeltaX<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr DeltaY<T> operator-(Height<T> lhs, Height<T> rhs) { return DeltaY<T>(lhs.as_value() - rhs.as_value()); }

// Multiplying by a scalar value is fine
template<typename T, typename Scalar>
inline constexpr Width<T> operator*(Scalar scale, Width<T> const& w) { return Width<T>{scale*w.as_value()}; }
template<typename T, typename Scalar>
inline constexpr Height<T> operator*(Scalar scale, Height<T> const& h) { return Height<T>{scale*h.as_value()}; }
template<typename T, typename Scalar>
inline constexpr DeltaX<T> operator*(Scalar scale, DeltaX<T> const& dx) { return DeltaX<T>{scale*dx.as_value()}; }
template<typename T, typename Scalar>
inline constexpr DeltaY<T> operator*(Scalar scale, DeltaY<T> const& dy) { return DeltaY<T>{scale*dy.as_value()}; }
template<typename T, typename Scalar>
inline constexpr Width<T> operator*(Width<T> const& w, Scalar scale) { return scale*w; }
template<typename T, typename Scalar>
inline constexpr Height<T> operator*(Height<T> const& h, Scalar scale) { return scale*h; }
template<typename T, typename Scalar>
inline constexpr DeltaX<T> operator*(DeltaX<T> const& dx, Scalar scale) { return scale*dx; }
template<typename T, typename Scalar>
inline constexpr DeltaY<T> operator*(DeltaY<T> const& dy, Scalar scale) { return scale*dy; }

// Dividing by a scaler value is fine
template<typename T, typename Scalar>
inline constexpr Width<T> operator/(Width<T> const& w, Scalar scale) { return Width<T>{w.as_value() / scale}; }
template<typename T, typename Scalar>
inline constexpr Height<T> operator/(Height<T> const& h, Scalar scale) { return Height<T>{h.as_value() / scale}; }
template<typename T, typename Scalar>
inline constexpr DeltaX<T> operator/(DeltaX<T> const& dx, Scalar scale) { return DeltaX<T>{dx.as_value() / scale}; }
template<typename T, typename Scalar>
inline constexpr DeltaY<T> operator/(DeltaY<T> const& dy, Scalar scale) { return DeltaY<T>{dy.as_value() / scale}; }
} // namespace

// Converting between types is fine, as long as they are along the same axis
template<typename T>
inline constexpr generic::Width<T> as_width(generic::DeltaX<T> const& dx) { return generic::Width<T>{dx.as_value()}; }
template<typename T>
inline constexpr generic::Height<T> as_height(generic::DeltaY<T> const& dy) { return generic::Height<T>{dy.as_value()}; }
template<typename T>
inline constexpr generic::X<T> as_x(generic::DeltaX<T> const& dx) { return generic::X<T>{dx.as_value()}; }
template<typename T>
inline constexpr generic::Y<T> as_y(generic::DeltaY<T> const& dy) { return generic::Y<T>{dy.as_value()}; }
template<typename T>
inline constexpr generic::DeltaX<T> as_delta(generic::X<T> const& x) { return generic::DeltaX<T>{x.as_value()}; }
template<typename T>
inline constexpr generic::DeltaY<T> as_delta(generic::Y<T> const& y) { return generic::DeltaY<T>{y.as_value()}; }
template<typename T>
inline constexpr generic::X<T> as_x(generic::Width<T> const& w) { return generic::X<T>{w.as_value()}; }
template<typename T>
inline constexpr generic::Y<T> as_y(generic::Height<T> const& h) { return generic::Y<T>{h.as_value()}; }
template<typename T>
inline constexpr generic::Width<T> as_width(generic::X<T> const& x) { return generic::Width<T>{x.as_value()}; }
template<typename T>
inline constexpr generic::Height<T> as_height(generic::Y<T> const& y) { return generic::Height<T>{y.as_value()}; }
template<typename T>
inline constexpr generic::DeltaX<T> as_delta(generic::Width<T> const& w) { return generic::DeltaX<T>{w.as_value()}; }
template<typename T>
inline constexpr generic::DeltaY<T> as_delta(generic::Height<T> const& h) { return generic::DeltaY<T>{h.as_value()}; }
} // namespace geometry
} // namespace mir

#endif // MIR_GEOMETRY_DIMENSIONS_H_
