/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_LAMBDA_AS_FUNCTION_H
#define MIRAL_LAMBDA_AS_FUNCTION_H

#include <functional>
#include <type_traits>

namespace miral
{
namespace detail
{
template <class F> struct FunctionType;

template<typename Lambda, typename Return, typename... Arg>
struct FunctionType<Return (Lambda::*)(Arg...)> { using type = std::function<Return(Arg...)>; };

template<typename Lambda, typename Return, typename... Arg>
struct FunctionType<Return (Lambda::*)(Arg...) const> { using type = std::function<Return(Arg...)>; };
}

template<typename Lambda>
auto lambda_as_function(Lambda&& lambda) -> typename detail::FunctionType<decltype(&std::remove_reference<Lambda>::type::operator())>::type
{
    return typename detail::FunctionType<decltype(&std::remove_reference<Lambda>::type::operator())>::type(std::forward<Lambda>(lambda));
}
}

#endif //MIRAL_LAMBDA_AS_FUNCTION_H
