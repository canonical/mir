/*
 * Copyright © Canonical Ltd.
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

#include <mir/geometry/dimensions.h>

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
        mir::geometry::generic::Value<float, Tag> precise,
        mir::geometry::generic::Value<int, Tag> discrete,
        mir::geometry::generic::Value<int, Tag> value120,
        bool stop)
        : precise{precise},
          discrete{discrete},
          value120{value120},
          stop{stop}
    {
    }

    ScrollAxisV1(
        mir::geometry::generic::Value<float, Tag> precise,
        mir::geometry::generic::Value<int, Tag> discrete,
        bool stop)
        : precise{precise},
          discrete{discrete},
          value120{discrete * 120},
          stop{stop}
    {
    }

    auto operator==(ScrollAxisV1 const& other) const -> bool {
        return precise  == other.precise &&
               discrete == other.discrete &&
               value120 == other.value120 &&
               stop     == other.stop;
    }

    auto operator!=(ScrollAxisV1 const& other) const -> bool {
        return !(*this == other);
    }

    mir::geometry::generic::Value<float, Tag> precise;
    mir::geometry::generic::Value<int, Tag> discrete;
    mir::geometry::generic::Value<int, Tag> value120;
    bool stop;
};

using ScrollAxisV1H = ScrollAxisV1<geometry::DeltaXTag>;
using ScrollAxisV1V = ScrollAxisV1<geometry::DeltaYTag>;

/// The current version of the ScrollAxis struct. When bumping to a new version make sure all existing ABI-stable
/// functions are changed to explicitly use the old version. The implementation of ABI-stable functions (unlike the
/// declaration) should already explicitly use a particlar version.
template<typename Tag>
using ScrollAxis = ScrollAxisV1<Tag>;
using ScrollAxisH = ScrollAxis<geometry::DeltaXTag>;
using ScrollAxisV = ScrollAxis<geometry::DeltaYTag>;

}
}

#endif // MIR_EVENTS_SCROLL_AXIS_H_
