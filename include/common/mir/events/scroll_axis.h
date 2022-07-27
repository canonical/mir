/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_EVENTS_SCROLL_AXIS_H_
#define MIR_EVENTS_SCROLL_AXIS_H_

#include "mir/geometry/dimensions.h"

namespace mir
{
namespace events
{

/// Information about a single scroll axis. To maintain ABI stability more versions may be added, in which
/// case older versions will be converted to the latest version on event construction.
template<typename Tag>
struct ScrollAxisV1
{
    ScrollAxisV1()
        : stop{false}
    {
    }

    ScrollAxisV1(
        mir::geometry::generic::Value<float>::Wrapper<Tag> precise,
        mir::geometry::generic::Value<int>::Wrapper<Tag> discrete,
        bool stop)
        : precise{precise},
          discrete{discrete},
          stop{stop}
    {
    }

    mir::geometry::generic::Value<float>::Wrapper<Tag> precise;
    mir::geometry::generic::Value<int>::Wrapper<Tag> discrete;
    bool stop;
};

template<typename Tag>
inline bool operator==(ScrollAxisV1<Tag> const& lhs, ScrollAxisV1<Tag> const & rhs)
{
    return lhs.precise == rhs.precise &&
           lhs.discrete == rhs.discrete &&
           lhs.stop == lhs.stop;
}

using ScrollAxisV1H = ScrollAxisV1<geometry::DeltaXTag>;
using ScrollAxisV1V = ScrollAxisV1<geometry::DeltaYTag>;

/// The current version of the ScrollAxis struct
template<typename Tag>
using ScrollAxis = ScrollAxisV1<Tag>;
using ScrollAxisH = ScrollAxis<geometry::DeltaXTag>;
using ScrollAxisV = ScrollAxis<geometry::DeltaYTag>;

}
}

#endif // MIR_EVENTS_SCROLL_AXIS_H_
