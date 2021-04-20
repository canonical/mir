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

/// These tag types determine what type of dimension a value holds and what operations are possible with it. They are
/// only used as template paramaters, are never instantiated and should only require forward declarations, but some
/// compiler versions seem to fail if they aren't given real declarations.
/// @{
struct WidthTag{};
struct HeightTag{};
struct XTag{};
struct YTag{};
struct DeltaXTag{};
struct DeltaYTag{};
/// @}

namespace detail
{
struct ValueWrapperBase{}; // Used for determining if a type is a wrapper
}

namespace generic
{
template<typename T>
struct Value
{
    /// Wraps a geometry value and prevents it from being accidentally used for invalid operations (such as setting a
    /// width to a height or adding two x positions together). Of course, explicit casts are possible to get around
    /// these restrictions (see the as_*() functions). int values (which are most values) should use the
    /// derived IntWrapper class, and other types should use this directly.
    template<typename Tag>
    struct Wrapper: detail::ValueWrapperBase
    {
        using ValueType = T;
        using TagType = Tag;
        template<typename OtherTag>
        using Corresponding = Wrapper<OtherTag>;

        template <typename Q = T>
        constexpr typename std::enable_if<std::is_integral<Q>::value, int>::type as_int() const
        {
            return this->value;
        }

        constexpr T as_value() const noexcept
        {
            return value;
        }

        constexpr Wrapper() noexcept : value{} {}

        Wrapper& operator=(Wrapper const& that) noexcept
        {
            value = that.value;
            return *this;
        }

        constexpr Wrapper(Wrapper const& that) noexcept
            : value{that.value}
        {
        }

        template<typename W, typename std::enable_if<std::is_same<typename W::TagType, Tag>::value, bool>::type = true>
        explicit constexpr Wrapper(W const& value) noexcept
            : value{static_cast<T>(value.as_value())}
        {
        }

        template<typename U, typename std::enable_if<std::is_scalar<U>::value, bool>::type = true>
        explicit constexpr Wrapper(U const& value) noexcept
            : value{static_cast<T>(value)}
        {
        }

        inline constexpr auto operator == (Wrapper<Tag> const& rhs) const -> bool
        {
            return value == rhs.as_value();
        }

        inline constexpr auto operator != (Wrapper<Tag> const& rhs) const -> bool
        {
            return value != rhs.as_value();
        }

        inline constexpr auto operator <= (Wrapper<Tag> const& rhs) const -> bool
        {
            return value <= rhs.as_value();
        }

        inline constexpr auto operator >= (Wrapper<Tag> const& rhs) const -> bool
        {
            return value >= rhs.as_value();
        }

        inline constexpr auto operator < (Wrapper<Tag> const& rhs) const -> bool
        {
            return value < rhs.as_value();
        }

        inline constexpr auto operator > (Wrapper<Tag> const& rhs) const -> bool
        {
            return value > rhs.as_value();
        }

    protected:
        T value;
    };

private:
    Value();
};

template<class GeometricType, typename Tag>
using Corresponding = typename GeometricType::template Corresponding<Tag>;

template<typename W, typename std::enable_if<std::is_base_of<detail::ValueWrapperBase, W>::value, bool>::type = true>
std::ostream& operator<<(std::ostream& out, W const& value)
{
    out << value.as_value();
    return out;
}

template<typename T> using Width = typename Value<T>::template Wrapper<WidthTag>;
template<typename T> using Height = typename Value<T>::template Wrapper<HeightTag>;
template<typename T> using X = typename Value<T>::template Wrapper<XTag>;
template<typename T> using Y = typename Value<T>::template Wrapper<YTag>;
template<typename T> using DeltaX = typename Value<T>::template Wrapper<DeltaXTag>;
template<typename T> using DeltaY = typename Value<T>::template Wrapper<DeltaYTag>;
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
