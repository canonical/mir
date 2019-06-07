/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIRAL_ZONE_H
#define MIRAL_ZONE_H

#include <mir_toolkit/common.h>

#include <mir/geometry/rectangle.h>
#include <mir/int_wrapper.h>

#include <memory>

namespace miral
{
using namespace mir::geometry;

/// A rectangular area of the display.
/// Not tied to a specific output.
class Zone
{
public:

    Zone(Rectangle const& extents); ///< Create a new zone with the given extents
    Zone(Zone const& other); ///< Makes a copy of the underlying private data
    Zone& operator=(Zone const& other); ///< Copies private data by value
    ~Zone();

    /// Returns false if any properties are different (even if they are the same zone)
    /// Will always return false if they are different zones (even if they have the same extents)
    auto operator==(Zone const& other) const -> bool;

    /// Multiple zone objects with different extents may be the "same" zone. For example, the arguments of
    /// miral::WindowManagementPolicy::advise_output_update() are old and new instances of the same zone, so
    /// updated.is_same_zone(original) will return true even though the extents may not be equal.
    auto is_same_zone(Zone const& other) const -> bool;

    /// The area of this zone in global display coordinates
    auto extents() const -> Rectangle;

    /// Set the extents of this zone
    /// Does not make this a different zone
    void extents(Rectangle const& extents);

private:
    class Self;
    std::unique_ptr<Self> self;
};
}

#endif // MIRAL_ZONE_H
