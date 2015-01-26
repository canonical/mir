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

#include "mir/pp_foreach.h"

#include <type_traits>
#include <string>
#include <ostream>

namespace mir
{

#define MIR_FLAGS_DECLARE_VALUE(x,y) x = y,
#define MIR_FLAGS_APPEND_VALUE(x,y)                                                              \
    if (((y) == 0 && flags.value == 0) ||                                                        \
        ((y) != 0 && (y) == (flags.value&(y))))                                                  \
    {                                                                                            \
        if(first)                                                                                \
        {                                                                                        \
            first = false;                                                                       \
            str = #x;                                                                            \
        }                                                                                        \
        else str += "|" #x;                                                                      \
    }
#define MIR_FLAGS_APPEND_ENUM(x)                                                                 \
    if ((type(x) == 0 && flags.value == 0) ||                                                    \
        (type(x) != 0 && type(x) == (flags.value&type(x))))                                      \
    {                                                                                            \
        if(first)                                                                                \
        {                                                                                        \
            first = false;                                                                       \
            str = #x;                                                                            \
        }                                                                                        \
        else str += "|" #x;                                                                      \
    }

/*!
 * Create pretty printing for an existing enumeration.
 *
 * Use this macro instead of MIR_FLAGS(...) if the enumeration already exists.
 * Usage example:
 * \code
 *     MIR_FLAGS(Proc,Zero,Carry,Interrupt);
 *     using ProcFlags = mir::Flags<Proc>;
 * \endcode
 *
 * If you use this for scoped enumerations you will have to repeat the enumeration type for each
 * value, hence those will also show up inside the strings.
 *
 * \note The bitwise (Enum,Enum) operators are declared inside mir namespace to make collisions
 * with other customer provided operators impossible.
 */
#define MIR_FLAGS_PRETTY_PRINTER(Type,...)                                                       \
namespace detail                                                                                 \
{                                                                                                \
   struct TypePrinter ##  Type                                                                   \
   {                                                                                             \
       inline std::string operator()(mir::Flags<Type> const& flags) const                        \
       {                                                                                         \
           bool first = true;                                                                    \
           using type = mir::Flags<Type>::value_type;                                            \
           std::string str("Empty");                                                             \
           MIR_FOR_EACH(MIR_FLAGS_APPEND_ENUM,__VA_ARGS__);                                      \
           return str;                                                                           \
       }                                                                                         \
   };                                                                                            \
}                                                                                                \
inline std::true_type has_type_printer_helper(Type);                                             \
inline detail::TypePrinter ## Type get_type_printer_helper(Type);                                \
inline std::true_type is_bit_enum_flag_helper(Type);                                             \

/*!
 * Declare a scoped enum the underlying type and let the macro add utilities to allow type safe
 * bit wise operations for that enum and pretty printing to a std::string using
 * mir::to_string(Flags<EnumType>);
 *
 * Usage example:
 * \code
 *     MIR_FLAGS(Proc,uint32_t,(Zero,1),(Carry,1),(Interrupt,2));
 *     using ProcFlags = mir::Flags<Proc>;
 * \endcode
 *
 * \note The bitwise (Enum,Enum) operators are declared inside mir namespace to make collisions
 * with other customer provided operators impossible.
 */
#define MIR_FLAGS(Type,UnderlyingType,...)                                                       \
enum class Type : UnderlyingType { MIR_FOR_EACH_TUPLE(MIR_FLAGS_DECLARE_VALUE,__VA_ARGS__) };    \
namespace detail                                                                                 \
{                                                                                                \
   struct TypePrinter ##  Type                                                                   \
   {                                                                                             \
       inline std::string operator()(mir::Flags<Type> const& flags) const                        \
       {                                                                                         \
           bool first = true;                                                                    \
           std::string str("Empty");                                                             \
           MIR_FOR_EACH_TUPLE(MIR_FLAGS_APPEND_VALUE,__VA_ARGS__);                               \
           return str;                                                                           \
       }                                                                                         \
   };                                                                                            \
}                                                                                                \
inline std::true_type has_type_printer_helper(Type);                                             \
inline detail::TypePrinter ## Type get_type_printer_helper(Type);                                \
inline std::true_type is_bit_enum_flag_helper(Type);                                             \


inline std::false_type has_type_printer_helper(...);
inline std::false_type is_bit_enum_flag_helper(...);

/*!
 * Treat an enumeration, scoped and unscoped, like a set of flags.
 *
 * If Enum | Enum and similar operations should yield Flags<Enum> declare
 * a function within the namespace of Enum:
 *   std::true_type is_bit_enum_flag(Enum);
 * and the respective operators in namespace mir will be enabled.
 *
 * This will be done auotmatially if the enumeration is declared with
 * the MIR_FLAGS macro.
 */
template<typename Enum>
struct Flags
{
    using value_type = typename std::underlying_type<Enum>::type;
    value_type value;

    explicit constexpr Flags(value_type value = 0) noexcept
        : value{value} {}
    constexpr Flags(Enum value) noexcept
        : value{static_cast<value_type>(value)} {}

    constexpr Flags<Enum> operator|(Flags<Enum> other) const noexcept
    {
        return Flags<Enum>(value|other.value);
    }

    constexpr Flags<Enum> operator&(Flags<Enum> other) const noexcept
    {
        return Flags<Enum>(value & other.value);
    }

    constexpr Flags<Enum> operator^(Flags<Enum> other) const noexcept
    {
        return Flags<Enum>(value ^ other.value);
    }

    constexpr Flags<Enum> operator~() const noexcept
    {
        return Flags<Enum>(~value);
    }

    // those mutating operators could be trated as constexpr with c++14
    Flags<Enum>& operator|=(Flags<Enum> other) noexcept
    {
        value |= other.value;
        return *this;
    }

    Flags<Enum> operator&=(Flags<Enum> other) noexcept
    {
        value &= other.value;
        return *this;
    }

    Flags<Enum> operator^=(Flags<Enum> other) noexcept
    {
        value ^= other.value;
        return *this;
    }

    constexpr bool operator==(Flags<Enum> other) const noexcept
    {
        return value == other.value;
    }
};

template<typename Enum>
typename std::enable_if<
     decltype(has_type_printer_helper(static_cast<Enum>(0)))::value,
     std::string
     >::type
     to_string(Flags<Enum> flags)
{
    decltype(get_type_printer_helper(static_cast<Enum>(0))) printer;
    return printer(flags);
}

template<typename Enum>
typename std::enable_if<
     decltype(has_type_printer_helper(static_cast<Enum>(0)))::value,
     std::ostream
     >::type
     operator<<(std::ostream& out, Flags<Enum> flags)
{
    return out << to_string(flags);
}

template<typename Enum>
constexpr Flags<Enum> operator|(Flags<Enum> flags, Enum e) noexcept
{
    return Flags<Enum>(flags.value | static_cast<decltype(flags.value)>(e));
}

template<typename Enum>
constexpr Flags<Enum> operator|(Enum e, Flags<Enum> flags) noexcept
{
    return Flags<Enum>(flags.value | static_cast<decltype(flags.value)>(e));
}

template<typename Enum>
constexpr Enum operator&(Enum e, Flags<Enum> flags) noexcept
{
    return static_cast<Enum>(flags.value & static_cast<decltype(flags.value)>(e));
}

template<typename Enum>
constexpr Enum operator&(Flags<Enum> flags, Enum e) noexcept
{
    return static_cast<Enum>(flags.value & static_cast<decltype(flags.value)>(e));
}

template<typename Enum>
constexpr bool operator==(Flags<Enum> flags, Enum e) noexcept
{
    return e == static_cast<Enum>(flags.value);
}

template<typename Enum>
constexpr bool operator==(Enum e, Flags<Enum> flags) noexcept
{
    return e == static_cast<Enum>(flags.value);
}

template<typename Enum>
constexpr bool contains(Flags<Enum> flags, Enum e) noexcept
{
    return e == static_cast<Enum>(flags.value & static_cast<decltype(flags.value)>(e));
}

template<typename T>
struct is_bit_flag_enum
{
    using type = decltype(is_bit_enum_flag_helper(static_cast<T>(0)));
};

template<typename T>
using is_bit_flag_enum_t = typename is_bit_flag_enum<T>::type;

template<typename Enum>
constexpr typename std::enable_if<is_bit_flag_enum_t<Enum>::value,Flags<Enum>>::type
operator|(Enum lhs, Enum rhs) noexcept
{
    return Flags<Enum>(lhs) | Flags<Enum>(rhs);
}

template<typename Enum>
constexpr typename std::enable_if<is_bit_flag_enum_t<Enum>::value,Flags<Enum>>::type
operator&(Enum lhs, Enum rhs) noexcept
{
    return Flags<Enum>(lhs) & Flags<Enum>(rhs);
}

template<typename Enum>
constexpr typename std::enable_if<is_bit_flag_enum_t<Enum>::value,Flags<Enum>>::type
operator^(Enum lhs, Enum rhs) noexcept
{
    return Flags<Enum>(lhs) ^ Flags<Enum>(rhs);
}

}

#endif
