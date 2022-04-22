/*
 * Copyright © 2012, 2016 Canonical Ltd.
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

#ifndef MIR_GEOMETRY_DIMENSIONS_H_
#define MIR_GEOMETRY_DIMENSIONS_H_

#include "dimensions_generic.h"

#include <cstdint>

namespace mir
{
/// Basic geometry types. Types for dimensions, displacements, etc.
/// and the operations that they support.
namespace geometry
{
namespace detail
{
template<typename Tag>
class IntWrapper : public generic::Value<int>::Wrapper<Tag>
{
public:
    template<typename OtherTag>
    using Corresponding = IntWrapper<OtherTag>;

    using generic::Value<int>::Wrapper<Tag>::Wrapper;

    constexpr IntWrapper() {}

    constexpr uint32_t as_uint32_t() const  // TODO: Deprecate this later
    {
        return (uint32_t)this->value;
    }
};
} // namespace detail

typedef detail::IntWrapper<WidthTag> Width;
typedef detail::IntWrapper<HeightTag> Height;
// Just to be clear, mir::geometry::Stride is the stride of the buffer in bytes
typedef detail::IntWrapper<struct StrideTag> Stride;

typedef detail::IntWrapper<XTag> X;
typedef detail::IntWrapper<YTag> Y;
typedef detail::IntWrapper<DeltaXTag> DeltaX;
typedef detail::IntWrapper<DeltaYTag> DeltaY;
}
}

#endif /* MIR_GEOMETRY_DIMENSIONS_H_ */
