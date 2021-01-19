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
    using ValueType = T;
    using TagType = Tag;
    template<typename OtherTag>
    using WrapperType = Wrapper<OtherTag, T>;

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
template<template<typename> typename T>
inline constexpr T<DeltaXTag> operator+(T<DeltaXTag> lhs, T<DeltaXTag> rhs){ return T<DeltaXTag>(lhs.as_value() + rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<DeltaYTag> operator+(T<DeltaYTag> lhs, T<DeltaYTag> rhs) { return T<DeltaYTag>(lhs.as_value() + rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<DeltaXTag> operator-(T<DeltaXTag> lhs, T<DeltaXTag> rhs) { return T<DeltaXTag>(lhs.as_value() - rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<DeltaYTag> operator-(T<DeltaYTag> lhs, T<DeltaYTag> rhs) { return T<DeltaYTag>(lhs.as_value() - rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<DeltaXTag> operator-(T<DeltaXTag> rhs) { return T<DeltaXTag>(-rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<DeltaYTag> operator-(T<DeltaYTag> rhs) { return T<DeltaYTag>(-rhs.as_value()); }
template<template<typename> typename T>
inline T<DeltaXTag>& operator+=(T<DeltaXTag>& lhs, T<DeltaXTag> rhs) { return lhs = lhs + rhs; }
template<template<typename> typename T>
inline T<DeltaYTag>& operator+=(T<DeltaYTag>& lhs, T<DeltaYTag> rhs) { return lhs = lhs + rhs; }
template<template<typename> typename T>
inline T<DeltaXTag>& operator-=(T<DeltaXTag>& lhs, T<DeltaXTag> rhs) { return lhs = lhs - rhs; }
template<template<typename> typename T>
inline T<DeltaYTag>& operator-=(T<DeltaYTag>& lhs, T<DeltaYTag> rhs) { return lhs = lhs - rhs; }

// Adding deltas to co-ordinates is fine
template<template<typename> typename T>
inline constexpr T<XTag> operator+(T<XTag> lhs, T<DeltaXTag> rhs) { return T<XTag>(lhs.as_value() + rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<YTag> operator+(T<YTag> lhs, T<DeltaYTag> rhs) { return T<YTag>(lhs.as_value() + rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<XTag> operator-(T<XTag> lhs, T<DeltaXTag> rhs) { return T<XTag>(lhs.as_value() - rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<YTag> operator-(T<YTag> lhs, T<DeltaYTag> rhs) { return T<YTag>(lhs.as_value() - rhs.as_value()); }
template<template<typename> typename T>
inline T<XTag>& operator+=(T<XTag>& lhs, T<DeltaXTag> rhs) { return lhs = lhs + rhs; }
template<template<typename> typename T>
inline T<YTag>& operator+=(T<YTag>& lhs, T<DeltaYTag> rhs) { return lhs = lhs + rhs; }
template<template<typename> typename T>
inline T<XTag>& operator-=(T<XTag>& lhs, T<DeltaXTag> rhs) { return lhs = lhs - rhs; }
template<template<typename> typename T>
inline T<YTag>& operator-=(T<YTag>& lhs, T<DeltaYTag> rhs) { return lhs = lhs - rhs; }

// Adding deltas to generic::Width and generic::Height is fine
template<template<typename> typename T>
inline constexpr T<WidthTag> operator+(T<WidthTag> lhs, T<DeltaXTag> rhs) { return T<WidthTag>(lhs.as_value() + rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<HeightTag> operator+(T<HeightTag> lhs, T<DeltaYTag> rhs) { return T<HeightTag>(lhs.as_value() + rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<WidthTag> operator-(T<WidthTag> lhs, T<DeltaXTag> rhs) { return T<WidthTag>(lhs.as_value() - rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<HeightTag> operator-(T<HeightTag> lhs, T<DeltaYTag> rhs) { return T<HeightTag>(lhs.as_value() - rhs.as_value()); }
template<template<typename> typename T>
inline T<WidthTag>& operator+=(T<WidthTag>& lhs, T<DeltaXTag> rhs) { return lhs = lhs + rhs; }
template<template<typename> typename T>
inline T<HeightTag>& operator+=(T<HeightTag>& lhs, T<DeltaYTag> rhs) { return lhs = lhs + rhs; }
template<template<typename> typename T>
inline T<WidthTag>& operator-=(T<WidthTag>& lhs, T<DeltaXTag> rhs) { return lhs = lhs - rhs; }
template<template<typename> typename T>
inline T<HeightTag>& operator-=(T<HeightTag>& lhs, T<DeltaYTag> rhs) { return lhs = lhs - rhs; }

// Adding Widths and Heights is fine
template<template<typename> typename T>
inline constexpr T<WidthTag> operator+(T<WidthTag> lhs, T<WidthTag> rhs) { return T<WidthTag>(lhs.as_value() + rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<HeightTag> operator+(T<HeightTag> lhs, T<HeightTag> rhs) { return T<HeightTag>(lhs.as_value() + rhs.as_value()); }
template<template<typename> typename T>
inline T<WidthTag>& operator+=(T<WidthTag>& lhs, T<WidthTag> rhs) { return lhs = lhs + rhs; }
template<template<typename> typename T>
inline T<HeightTag>& operator+=(T<HeightTag>& lhs, T<HeightTag> rhs) { return lhs = lhs + rhs; }

// Subtracting coordinates is fine
template<template<typename> typename T>
inline constexpr T<DeltaXTag> operator-(T<XTag> lhs, T<XTag> rhs) { return T<DeltaXTag>(lhs.as_value() - rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<DeltaYTag> operator-(T<YTag> lhs, T<YTag> rhs) { return T<DeltaYTag>(lhs.as_value() - rhs.as_value()); }

//Subtracting T<WidthTag> and T<HeightTag> is fine
template<template<typename> typename T>
inline constexpr T<DeltaXTag> operator-(T<WidthTag> lhs, T<WidthTag> rhs) { return T<DeltaXTag>(lhs.as_value() - rhs.as_value()); }
template<template<typename> typename T>
inline constexpr T<DeltaYTag> operator-(T<HeightTag> lhs, T<HeightTag> rhs) { return T<DeltaYTag>(lhs.as_value() - rhs.as_value()); }

// Multiplying by a scalar value is fine
template<template<typename> typename T, typename Scalar>
inline constexpr T<WidthTag> operator*(Scalar scale, T<WidthTag> const& w) { return T<WidthTag>{scale*w.as_value()}; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<HeightTag> operator*(Scalar scale, T<HeightTag> const& h) { return T<HeightTag>{scale*h.as_value()}; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<DeltaXTag> operator*(Scalar scale, T<DeltaXTag> const& dx) { return T<DeltaXTag>{scale*dx.as_value()}; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<DeltaYTag> operator*(Scalar scale, T<DeltaYTag> const& dy) { return T<DeltaYTag>{scale*dy.as_value()}; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<WidthTag> operator*(T<WidthTag> const& w, Scalar scale) { return scale*w; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<HeightTag> operator*(T<HeightTag> const& h, Scalar scale) { return scale*h; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<DeltaXTag> operator*(T<DeltaXTag> const& dx, Scalar scale) { return scale*dx; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<DeltaYTag> operator*(T<DeltaYTag> const& dy, Scalar scale) { return scale*dy; }

// Dividing by a scaler value is fine
template<template<typename> typename T, typename Scalar>
inline constexpr T<WidthTag> operator/(T<WidthTag> const& w, Scalar scale) { return T<WidthTag>{w.as_value() / scale}; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<HeightTag> operator/(T<HeightTag> const& h, Scalar scale) { return T<HeightTag>{h.as_value() / scale}; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<DeltaXTag> operator/(T<DeltaXTag> const& dx, Scalar scale) { return T<DeltaXTag>{dx.as_value() / scale}; }
template<template<typename> typename T, typename Scalar>
inline constexpr T<DeltaYTag> operator/(T<DeltaYTag> const& dy, Scalar scale) { return T<DeltaYTag>{dy.as_value() / scale}; }

// Converting between types is fine, as long as they are along the same axis
template<template<typename> typename T>
inline constexpr T<WidthTag> as_width(T<DeltaXTag> const& dx) { return T<WidthTag>{dx.as_value()}; }
template<template<typename> typename T>
inline constexpr T<HeightTag> as_height(T<DeltaYTag> const& dy) { return T<HeightTag>{dy.as_value()}; }
template<template<typename> typename T>
inline constexpr T<XTag> as_x(T<DeltaXTag> const& dx) { return T<XTag>{dx.as_value()}; }
template<template<typename> typename T>
inline constexpr T<YTag> as_y(T<DeltaYTag> const& dy) { return T<YTag>{dy.as_value()}; }
template<template<typename> typename T>
inline constexpr T<DeltaXTag> as_delta(T<XTag> const& x) { return T<DeltaXTag>{x.as_value()}; }
template<template<typename> typename T>
inline constexpr T<DeltaYTag> as_delta(T<YTag> const& y) { return T<DeltaYTag>{y.as_value()}; }
template<template<typename> typename T>
inline constexpr T<XTag> as_x(T<WidthTag> const& w) { return T<XTag>{w.as_value()}; }
template<template<typename> typename T>
inline constexpr T<YTag> as_y(T<HeightTag> const& h) { return T<YTag>{h.as_value()}; }
template<template<typename> typename T>
inline constexpr T<WidthTag> as_width(T<XTag> const& x) { return T<WidthTag>{x.as_value()}; }
template<template<typename> typename T>
inline constexpr T<HeightTag> as_height(T<YTag> const& y) { return T<HeightTag>{y.as_value()}; }
template<template<typename> typename T>
inline constexpr T<DeltaXTag> as_delta(T<WidthTag> const& w) { return T<DeltaXTag>{w.as_value()}; }
template<template<typename> typename T>
inline constexpr T<DeltaYTag> as_delta(T<HeightTag> const& h) { return T<DeltaYTag>{h.as_value()}; }

template<typename Target, typename Source>
inline constexpr Target dim_cast(Source s) { return Target(s.as_value()); }
} // namespace geometry
} // namespace mir

#endif // MIR_GEOMETRY_DIMENSIONS_GENERIC_H_
