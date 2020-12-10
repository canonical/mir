/*
 * Copyright © 2020 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_GEOMETRY_DIMENSIONS_GENERIC_H_
#define MIR_GEOMETRY_DIMENSIONS_GENERIC_H_

#include <iosfwd>
#include <type_traits>

namespace mir
{
/// Basic geometry types. Types for dimensions, displacements, etc.
/// and the operations that they support.
namespace geometry
{

struct WidthTag;
struct HeightTag;
struct XTag;
struct YTag;
struct DeltaXTag;
struct DeltaYTag;

namespace generic
{
namespace detail
{
template<typename Tag, typename T>
class Wrapper
{
public:
    typedef T ValueType;

    constexpr Wrapper() : value{} {}

    template<typename U>
    Wrapper& operator=(Wrapper<Tag, U> const& that)
    {
        value = static_cast<T>(that.value);
    }

    template<typename U>
    constexpr Wrapper(Wrapper<Tag, U> const& value)
        : value{static_cast<T>(value.as_value())}
    {
    }

    template<typename U>
    explicit constexpr Wrapper(U const& value)
        : value{static_cast<T>(value)}
    {
    }

    template <typename Q = T>
    constexpr typename std::enable_if<std::is_integral<Q>::value, int>::type as_int() const
    {
        return this->value;
    }

    constexpr T as_value() const
    {
        return value;
    }

protected:
    T value;
};

template<typename Tag, typename T>
std::ostream& operator<<(std::ostream& out, Wrapper<Tag, T> const& value)
{
    out << value.as_value();
    return out;
}

template<typename Tag, typename T>
inline constexpr bool operator == (Wrapper<Tag, T> const& lhs, Wrapper<Tag, T> const& rhs)
{
    return lhs.as_value() == rhs.as_value();
}

template<typename Tag, typename T>
inline constexpr bool operator != (Wrapper<Tag, T> const& lhs, Wrapper<Tag, T> const& rhs)
{
    return lhs.as_value() != rhs.as_value();
}

template<typename Tag, typename T>
inline constexpr bool operator <= (Wrapper<Tag, T> const& lhs, Wrapper<Tag, T> const& rhs)
{
    return lhs.as_value() <= rhs.as_value();
}

template<typename Tag, typename T>
inline constexpr bool operator >= (Wrapper<Tag, T> const& lhs, Wrapper<Tag, T> const& rhs)
{
    return lhs.as_value() >= rhs.as_value();
}

template<typename Tag, typename T>
inline constexpr bool operator < (Wrapper<Tag, T> const& lhs, Wrapper<Tag, T> const& rhs)
{
    return lhs.as_value() < rhs.as_value();
}

template<typename Tag, typename T>
inline constexpr bool operator > (Wrapper<Tag, T> const& lhs, Wrapper<Tag, T> const& rhs)
{
    return lhs.as_value() > rhs.as_value();
}
} // namespace detail

template<typename T> using Width = detail::Wrapper<WidthTag, T>;
template<typename T> using Height = detail::Wrapper<HeightTag, T>;
template<typename T> using X = detail::Wrapper<XTag, T>;
template<typename T> using Y = detail::Wrapper<YTag, T>;
template<typename T> using DeltaX = detail::Wrapper<DeltaXTag, T>;
template<typename T> using DeltaY = detail::Wrapper<DeltaYTag, T>;
} // namespace generic

// Adding deltas is fine
template<typename T>
inline constexpr generic::DeltaX<T> operator+(generic::DeltaX<T> lhs, generic::DeltaX<T> rhs){ return generic::DeltaX<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr generic::DeltaY<T> operator+(generic::DeltaY<T> lhs, generic::DeltaY<T> rhs) { return generic::DeltaY<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr generic::DeltaX<T> operator-(generic::DeltaX<T> lhs, generic::DeltaX<T> rhs) { return generic::DeltaX<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr generic::DeltaY<T> operator-(generic::DeltaY<T> lhs, generic::DeltaY<T> rhs) { return generic::DeltaY<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr generic::DeltaX<T> operator-(generic::DeltaX<T> rhs) { return generic::DeltaX<T>(-rhs.as_value()); }
template<typename T>
inline constexpr generic::DeltaY<T> operator-(generic::DeltaY<T> rhs) { return generic::DeltaY<T>(-rhs.as_value()); }
template<typename T>
inline generic::DeltaX<T>& operator+=(generic::DeltaX<T>& lhs, generic::DeltaX<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline generic::DeltaY<T>& operator+=(generic::DeltaY<T>& lhs, generic::DeltaY<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline generic::DeltaX<T>& operator-=(generic::DeltaX<T>& lhs, generic::DeltaX<T> rhs) { return lhs = lhs - rhs; }
template<typename T>
inline generic::DeltaY<T>& operator-=(generic::DeltaY<T>& lhs, generic::DeltaY<T> rhs) { return lhs = lhs - rhs; }

// Adding deltas to co-ordinates is fine
template<typename T>
inline constexpr generic::X<T> operator+(generic::X<T> lhs, generic::DeltaX<T> rhs) { return generic::X<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr generic::Y<T> operator+(generic::Y<T> lhs, generic::DeltaY<T> rhs) { return generic::Y<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr generic::X<T> operator-(generic::X<T> lhs, generic::DeltaX<T> rhs) { return generic::X<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr generic::Y<T> operator-(generic::Y<T> lhs, generic::DeltaY<T> rhs) { return generic::Y<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline generic::X<T>& operator+=(generic::X<T>& lhs, generic::DeltaX<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline generic::Y<T>& operator+=(generic::Y<T>& lhs, generic::DeltaY<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline generic::X<T>& operator-=(generic::X<T>& lhs, generic::DeltaX<T> rhs) { return lhs = lhs - rhs; }
template<typename T>
inline generic::Y<T>& operator-=(generic::Y<T>& lhs, generic::DeltaY<T> rhs) { return lhs = lhs - rhs; }

// Adding deltas to generic::Width and generic::Height is fine
template<typename T>
inline constexpr generic::Width<T> operator+(generic::Width<T> lhs, generic::DeltaX<T> rhs) { return generic::Width<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr generic::Height<T> operator+(generic::Height<T> lhs, generic::DeltaY<T> rhs) { return generic::Height<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr generic::Width<T> operator-(generic::Width<T> lhs, generic::DeltaX<T> rhs) { return generic::Width<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr generic::Height<T> operator-(generic::Height<T> lhs, generic::DeltaY<T> rhs) { return generic::Height<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline generic::Width<T>& operator+=(generic::Width<T>& lhs, generic::DeltaX<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline generic::Height<T>& operator+=(generic::Height<T>& lhs, generic::DeltaY<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline generic::Width<T>& operator-=(generic::Width<T>& lhs, generic::DeltaX<T> rhs) { return lhs = lhs - rhs; }
template<typename T>
inline generic::Height<T>& operator-=(generic::Height<T>& lhs, generic::DeltaY<T> rhs) { return lhs = lhs - rhs; }

// Adding Widths and Heights is fine
template<typename T>
inline constexpr generic::Width<T> operator+(generic::Width<T> lhs, generic::Width<T> rhs) { return generic::Width<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline constexpr generic::Height<T> operator+(generic::Height<T> lhs, generic::Height<T> rhs) { return generic::Height<T>(lhs.as_value() + rhs.as_value()); }
template<typename T>
inline generic::Width<T>& operator+=(generic::Width<T>& lhs, generic::Width<T> rhs) { return lhs = lhs + rhs; }
template<typename T>
inline generic::Height<T>& operator+=(generic::Height<T>& lhs, generic::Height<T> rhs) { return lhs = lhs + rhs; }

// Subtracting coordinates is fine
template<typename T>
inline constexpr generic::DeltaX<T> operator-(generic::X<T> lhs, generic::X<T> rhs) { return generic::DeltaX<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr generic::DeltaY<T> operator-(generic::Y<T> lhs, generic::Y<T> rhs) { return generic::DeltaY<T>(lhs.as_value() - rhs.as_value()); }

//Subtracting generic::Width<T> and generic::Height<T> is fine
template<typename T>
inline constexpr generic::DeltaX<T> operator-(generic::Width<T> lhs, generic::Width<T> rhs) { return generic::DeltaX<T>(lhs.as_value() - rhs.as_value()); }
template<typename T>
inline constexpr generic::DeltaY<T> operator-(generic::Height<T> lhs, generic::Height<T> rhs) { return generic::DeltaY<T>(lhs.as_value() - rhs.as_value()); }

// Multiplying by a scalar value is fine
template<typename T, typename Scalar>
inline constexpr generic::Width<T> operator*(Scalar scale, generic::Width<T> const& w) { return generic::Width<T>{scale*w.as_value()}; }
template<typename T, typename Scalar>
inline constexpr generic::Height<T> operator*(Scalar scale, generic::Height<T> const& h) { return generic::Height<T>{scale*h.as_value()}; }
template<typename T, typename Scalar>
inline constexpr generic::DeltaX<T> operator*(Scalar scale, generic::DeltaX<T> const& dx) { return generic::DeltaX<T>{scale*dx.as_value()}; }
template<typename T, typename Scalar>
inline constexpr generic::DeltaY<T> operator*(Scalar scale, generic::DeltaY<T> const& dy) { return generic::DeltaY<T>{scale*dy.as_value()}; }
template<typename T, typename Scalar>
inline constexpr generic::Width<T> operator*(generic::Width<T> const& w, Scalar scale) { return scale*w; }
template<typename T, typename Scalar>
inline constexpr generic::Height<T> operator*(generic::Height<T> const& h, Scalar scale) { return scale*h; }
template<typename T, typename Scalar>
inline constexpr generic::DeltaX<T> operator*(generic::DeltaX<T> const& dx, Scalar scale) { return scale*dx; }
template<typename T, typename Scalar>
inline constexpr generic::DeltaY<T> operator*(generic::DeltaY<T> const& dy, Scalar scale) { return scale*dy; }

// Dividing by a scaler value is fine
template<typename T, typename Scalar>
inline constexpr generic::Width<T> operator/(generic::Width<T> const& w, Scalar scale) { return generic::Width<T>{w.as_value() / scale}; }
template<typename T, typename Scalar>
inline constexpr generic::Height<T> operator/(generic::Height<T> const& h, Scalar scale) { return generic::Height<T>{h.as_value() / scale}; }
template<typename T, typename Scalar>
inline constexpr generic::DeltaX<T> operator/(generic::DeltaX<T> const& dx, Scalar scale) { return generic::DeltaX<T>{dx.as_value() / scale}; }
template<typename T, typename Scalar>
inline constexpr generic::DeltaY<T> operator/(generic::DeltaY<T> const& dy, Scalar scale) { return generic::DeltaY<T>{dy.as_value() / scale}; }

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

template<typename Target, typename Source>
inline constexpr Target dim_cast(Source s) { return Target(s.as_value()); }
} // namespace geometry
} // namespace mir

#endif // MIR_GEOMETRY_DIMENSIONS_GENERIC_H_
