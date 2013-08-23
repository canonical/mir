/*
 * Copyright © 2012, 2013 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_INT_WRAPPER_H_
#define MIR_INT_WRAPPER_H_

#include <iosfwd>

namespace mir
{
template<typename Tag>
class IntWrapper
{
public:
    typedef int ValueType;

    IntWrapper() : value(0) {}

    explicit IntWrapper(ValueType value) : value(value) {}
    ValueType as_value() const { return value; }

private:
    ValueType value;
};

template<typename Tag>
std::ostream& operator<<(std::ostream& out, IntWrapper<Tag> const& value)
{
    out << value.as_value();
    return out;
}

template<typename Tag>
inline bool operator == (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_value() == rhs.as_value();
}

template<typename Tag>
inline bool operator != (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_value() != rhs.as_value();
}

template<typename Tag>
inline bool operator <= (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_value() <= rhs.as_value();
}

template<typename Tag>
inline bool operator >= (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_value() >= rhs.as_value();
}

template<typename Tag>
inline bool operator < (IntWrapper<Tag> const& lhs, IntWrapper<Tag> const& rhs)
{
    return lhs.as_value() < rhs.as_value();
}
}

#include <functional>
namespace std
{
template<typename Tag>
struct hash<::mir::IntWrapper<Tag>>
{
    std::hash<int> self;
    std::size_t operator()(::mir::IntWrapper<Tag> const& id) const
    {
        return self(id.as_value());
    }
};
}

#endif // MIR_INT_WRAPPER_H_
