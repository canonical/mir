/*
 * Copyright ©, 2016 Canonical Ltd.
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

#ifndef MIR_INT_WRAPPER_H_
#define MIR_INT_WRAPPER_H_

#include <format>
#include <iosfwd>

namespace mir
{
template<typename Tag, typename ValueType = int>
class IntWrapper
{
public:
    constexpr IntWrapper() : value(0) {}

    explicit constexpr IntWrapper(ValueType value) : value(value) {}
    ValueType constexpr as_value() const { return value; }
    friend auto operator<=>(IntWrapper const& lhs, IntWrapper const& rhs) = default;

private:
    ValueType value;
};

template<typename Tag, typename ValueType>
std::ostream& operator<<(std::ostream& out, IntWrapper<Tag, ValueType> const& value)
{
    out << value.as_value();
    return out;
}
}

#include <functional>
namespace std
{
template<typename Tag, typename ValueType>
struct hash<::mir::IntWrapper<Tag, ValueType>>
{
    std::hash<int> self;
    constexpr std::size_t operator()(::mir::IntWrapper<Tag, ValueType> const& id) const { return self(id.as_value()); }
};

template<typename Tag, typename ValueType>
struct formatter<::mir::IntWrapper<Tag, ValueType>> : formatter<ValueType>
{
    auto format(::mir::IntWrapper<Tag, ValueType> const& value, std::format_context& ctx) const
        -> std::format_context::iterator
    { return formatter<ValueType>::format(value.as_value(), ctx); }
};
}

#endif // MIR_INT_WRAPPER_H_
