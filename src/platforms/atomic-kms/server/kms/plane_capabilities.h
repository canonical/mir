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

#ifndef MIR_GRAPHICS_ATOMIC_PLANE_CAPABILITIES_H_
#define MIR_GRAPHICS_ATOMIC_PLANE_CAPABILITIES_H_

#include <compare>
#include <cstdint>
#include <optional>
#include <vector>

namespace mir::graphics::atomic
{
/// A value to program into a DRM plane property, e.g. the value of a named enum entry
using PropertyValue = uint64_t;

struct FormatModifier
{
    uint32_t format;
    uint64_t modifier;

    auto operator<=>(FormatModifier const&) const = default;
};

struct ZPos
{
    int64_t min;
    int64_t max;
    int64_t current;
    bool immutable;

    auto operator==(ZPos const&) const -> bool = default;
};

/// Enum values to program for BT.709 limited-range YCbCr
struct Bt709Limited
{
    PropertyValue encoding;  ///< "COLOR_ENCODING" value named "ITU-R BT.709 YCbCr"
    PropertyValue range;     ///< "COLOR_RANGE" value named "YCbCr limited range"

    auto operator==(Bt709Limited const&) const -> bool = default;
};

struct PlaneCapabilities
{
    uint32_t id;

    /// Only the primary may lack zpos; it is then assumed to be below every overlay
    std::optional<ZPos> zpos;

    /// The exact (format, modifier) pairs from IN_FORMATS. Never empty
    std::vector<FormatModifier> formats;

    /// Maximum of the "alpha" range property, if the plane has one
    std::optional<PropertyValue> alpha_max;

    /// The "pixel blend mode" enum value named "None", if the plane has that property
    std::optional<PropertyValue> blend_none;

    /// Set when the plane lacks "alpha" or "pixel blend mode", so content on it is
    /// assumed to be shown fully opaque
    bool fixed_opaque;

    /// Present only when both color properties can program BT.709 limited range
    std::optional<Bt709Limited> bt709_limited;

    /// Legal stacking relative to the primary's current zpos. Only meaningful for overlays
    bool can_be_above_primary;
    bool can_be_below_primary;
};

struct CrtcPlaneCapabilities
{
    uint32_t crtc_id;
    uint32_t crtc_index;
    PlaneCapabilities primary;

    /// Usable overlays routable to this CRTC, ordered by (zpos.current, id).
    /// Includes overlays that can only sit below the primary
    std::vector<PlaneCapabilities> overlays;
};

/// Reads the plane capabilities of the planes routable to \a crtc_id.
///
/// Overlays with missing or malformed required properties are left out of the
/// result. Throws if the CRTC or a usable primary plane cannot be found.
auto discover_plane_capabilities(int drm_fd, uint32_t crtc_id) -> CrtcPlaneCapabilities;
}

#endif // MIR_GRAPHICS_ATOMIC_PLANE_CAPABILITIES_H_
