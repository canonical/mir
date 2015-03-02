/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_FLAGS_H_
#define MIR_FLAGS_H_

#include <type_traits>

namespace mir
{

/*!
 * Treat an enumeration, scoped and unscoped, like a set of flags.
 *
 * For scoped enumerations, there are optional bitwise operators available
 * that can be enabled by declaring a function within the namespace of the
 * enumeration (here Enum):
 * \begincode
 *   Enum mir_enable_enum_bit_operators(Enum);
 * \endcode
 */
template<typename Enum>
struct Flags
{
    using value_type = typename std::underlying_type<Enum>::type;

    explicit constexpr Flags(value_type flag_value = 0) noexcept
        : flag_value{flag_value} {}
    constexpr Flags(Enum flag_value) noexcept
        : flag_value{static_cast<value_type>(flag_value)} {}

    constexpr Flags<Enum> operator|(Flags<Enum> other) const noexcept
    {
        return Flags<Enum>(flag_value|other.flag_value);
    }

    constexpr Flags<Enum> operator&(Flags<Enum> other) const noexcept
    {
        return Flags<Enum>(flag_value & other.flag_value);
    }

    constexpr Flags<Enum> operator^(Flags<Enum> other) const noexcept
    {
        return Flags<Enum>(flag_value ^ other.flag_value);
    }

    // those mutating operators could be trated as constexpr with c++14
    Flags<Enum>& operator|=(Flags<Enum> other) noexcept
    {
        flag_value |= other.flag_value;
        return *this;
    }

    Flags<Enum> operator&=(Flags<Enum> other) noexcept
    {
        flag_value &= other.flag_value;
        return *this;
    }

    Flags<Enum> operator^=(Flags<Enum> other) noexcept
    {
        flag_value ^= other.flag_value;
        return *this;
    }

    constexpr bool operator==(Flags<Enum> other) const noexcept
    {
        return flag_value == other.flag_value;
    }

    constexpr value_type value() const noexcept
    {
        return flag_value;
    }

private:
    value_type flag_value;
};

template<typename Enum>
constexpr Flags<Enum> operator|(Flags<Enum> flags, Enum e) noexcept
{
    return Flags<Enum>(flags.value() | static_cast<decltype(flags.value())>(e));
}

template<typename Enum>
constexpr Flags<Enum> operator|(Enum e, Flags<Enum> flags) noexcept
{
    return Flags<Enum>(flags.value() | static_cast<decltype(flags.value())>(e));
}

template<typename Enum>
constexpr Enum operator&(Enum e, Flags<Enum> flags) noexcept
{
    return static_cast<Enum>(flags.value() & static_cast<decltype(flags.value())>(e));
}

template<typename Enum>
constexpr Enum operator&(Flags<Enum> flags, Enum e) noexcept
{
    return static_cast<Enum>(flags.value() & static_cast<decltype(flags.value())>(e));
}

template<typename Enum>
constexpr bool operator==(Flags<Enum> flags, Enum e) noexcept
{
    return e == static_cast<Enum>(flags.value());
}

template<typename Enum>
constexpr bool operator==(Enum e, Flags<Enum> flags) noexcept
{
    return e == static_cast<Enum>(flags.value());
}

template<typename Enum>
constexpr bool contains(Flags<Enum> flags, Enum e) noexcept
{
    return e == static_cast<Enum>(flags.value() & static_cast<decltype(flags.value())>(e));
}

}

template<typename Enum>
constexpr mir::Flags<decltype(mir_enable_enum_bit_operators(static_cast<Enum>(0)))>
operator|(Enum lhs, Enum rhs) noexcept
{
    return mir::Flags<Enum>(lhs) | mir::Flags<Enum>(rhs);
}

template<typename Enum>
constexpr mir::Flags<decltype(mir_enable_enum_bit_operators(static_cast<Enum>(0)))>
operator&(Enum lhs, Enum rhs) noexcept
{
    return mir::Flags<Enum>(lhs) & mir::Flags<Enum>(rhs);
}


template<typename Enum>
constexpr mir::Flags<decltype(mir_enable_enum_bit_operators(static_cast<Enum>(0)))>
operator^(Enum lhs, Enum rhs) noexcept
{
    return mir::Flags<Enum>(lhs) ^ mir::Flags<Enum>(rhs);
}

#endif
