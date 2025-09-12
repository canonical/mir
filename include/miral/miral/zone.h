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
///
/// For example, the area of an output into which applications are placed.
///
/// \sa miral::WindowManagementPolicy::advise_application_zone_create - notification of a new zone
class Zone
{
public:

    /// Construct a new zone with the given \p extents.
    ///
    /// \param extents area of the zone
    Zone(Rectangle const& extents);

    /// Construct a copy of another zone.
    ///
    /// \param other zone to copy
    Zone(Zone const& other);

    /// Copy from another zone.
    ///
    /// \param other zone to copy
    Zone& operator=(Zone const& other);
    ~Zone();

    /// Returns `true` only if all properties including IDs match.
    ///
    /// \returns `true` if it is a match, otherwise `false`
    auto operator==(Zone const& other) const -> bool;

    /// Returns `true` if zone IDs match, even if extents are different.
    ///
    /// \returns `true` if zone IDs match, otherwise `false`
    auto is_same_zone(Zone const& other) const -> bool;

    /// The area of this zone in global display coordinates.
    ///
    /// \returns the extends of the zone
    auto extents() const -> Rectangle;

    /// Set the extents of this zone.
    ///
    /// This does not make this a different zone, meaning that the #miral::Zone::id
    /// will remain the same.
    ///
    /// \param extents the new extends
    void extents(Rectangle const& extents);

    /// A unique identifier for this zone, which remains the same regardless of
    /// how the zone is resized and moved.
    ///
    /// \returns the unique id of the zone
    auto id() const -> int;

private:
    class Self;
    std::unique_ptr<Self> self;
};
}

#endif // MIRAL_ZONE_H
