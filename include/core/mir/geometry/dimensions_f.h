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

#ifndef MIR_GEOMETRY_DIMENSIONS_F_H_
#define MIR_GEOMETRY_DIMENSIONS_F_H_

#include "dimensions_generic.h"

namespace mir
{
namespace geometry
{
namespace detail
{
template<typename Tag>
class FloatWrapper : public generic::detail::Wrapper<Tag, float>
{
public:
    template<typename OtherTag>
    using WrapperType = FloatWrapper<OtherTag>;

    constexpr FloatWrapper() {}

    template<typename U>
    constexpr FloatWrapper(generic::detail::Wrapper<Tag, U> const& value)
        : generic::detail::Wrapper<Tag, int>{value}
    {
    }

    template<typename U>
    explicit constexpr FloatWrapper(U const& value)
        : generic::detail::Wrapper<Tag, int>{value}
    {
    }

    constexpr auto as_float() const -> float
    {
        return this->value;
    }
};
} // namespace detail

typedef detail::FloatWrapper<WidthTag> FWidth;
typedef detail::FloatWrapper<HeightTag> FHeight;
typedef detail::FloatWrapper<XTag> FX;
typedef detail::FloatWrapper<YTag> FY;
typedef detail::FloatWrapper<DeltaXTag> FDeltaX;
typedef detail::FloatWrapper<DeltaYTag> FDeltaY;
}
}

#endif // MIR_GEOMETRY_DIMENSIONS_F_H_
