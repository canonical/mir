/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_INT_WRAPPER_H_
#define MIR_INT_WRAPPER_H_

#include <iosfwd>

namespace mir
{
enum class IntWrapperTypeTag
{
    SessionsSurfaceId,
    GraphicsConfCardId,
    GraphicsConfOutputId
};

template<IntWrapperTypeTag Tag, typename ValueType_ = int>
class IntWrapper
{
public:
    typedef ValueType_ ValueType;

    IntWrapper() : value(0) {}

    explicit IntWrapper(ValueType value) : value(value) {}
    ValueType as_value() const { return value; }

private:
    ValueType value;
};

template<IntWrapperTypeTag Tag, typename ValueType>
std::ostream& operator<<(std::ostream& out, IntWrapper<Tag,ValueType> const& value)
{
    out << value.as_value();
    return out;
}

template<IntWrapperTypeTag Tag, typename ValueType>
inline bool operator == (IntWrapper<Tag,ValueType> const& lhs, IntWrapper<Tag,ValueType> const& rhs)
{
    return lhs.as_value() == rhs.as_value();
}

template<IntWrapperTypeTag Tag, typename ValueType>
inline bool operator != (IntWrapper<Tag,ValueType> const& lhs, IntWrapper<Tag,ValueType> const& rhs)
{
    return lhs.as_value() != rhs.as_value();
}

template<IntWrapperTypeTag Tag, typename ValueType>
inline bool operator <= (IntWrapper<Tag,ValueType> const& lhs, IntWrapper<Tag,ValueType> const& rhs)
{
    return lhs.as_value() <= rhs.as_value();
}

template<IntWrapperTypeTag Tag, typename ValueType>
inline bool operator >= (IntWrapper<Tag,ValueType> const& lhs, IntWrapper<Tag,ValueType> const& rhs)
{
    return lhs.as_value() >= rhs.as_value();
}

template<IntWrapperTypeTag Tag, typename ValueType>
inline bool operator < (IntWrapper<Tag,ValueType> const& lhs, IntWrapper<Tag,ValueType> const& rhs)
{
    return lhs.as_value() < rhs.as_value();
}
}

#endif // MIR_INT_WRAPPER_H_
