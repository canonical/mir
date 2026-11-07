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

    auto operator==(FormatModifier const&) const -> bool = default;
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

struct PlaneCapability
{
    uint32_t id;

    /// The plane's "zpos" range property. Required: a plane without a well-formed
    /// zpos is unusable
    ZPos zpos;

    /// The exact (format, modifier) pairs from IN_FORMATS. Never empty
    std::vector<FormatModifier> formats;

    /// Maximum of the "alpha" range property, if the plane has one
    std::optional<PropertyValue> alpha_max;

    /// The "pixel blend mode" enum value named "None", if the plane has that property and it has a None entry.
    std::optional<PropertyValue> blend_none;

    /// True when the plane has neither an "alpha" nor a "pixel blend mode"
    /// property, so content on it is assumed to be shown fully opaque
    bool fixed_opaque;

    /// Present only when both color properties can program BT.709 limited range
    std::optional<Bt709Limited> bt709_limited;

    /// Legal stacking relative to the primary's current zpos. Ties in zpos are
    /// broken by plane ID, matching drm_atomic_normalize_zpos(): the higher ID
    /// is above. Only meaningful for overlays
    bool can_be_above_primary{};
    bool can_be_below_primary{};
};

/// Describes the capabilities of the planes associated with \a crtc_id.
struct CrtcPlaneCapabilities
{
    uint32_t crtc_id;
    PlaneCapability primary;

    /// Usable overlays routable to this CRTC, ordered by (zpos.current, id).
    std::vector<PlaneCapability> overlays;
};

/// Reads the plane capabilities of the planes routable to \a crtc_id.
///
/// Only devices with DRM_CAP_ADDFB2_MODIFIERS are supported. IN_FORMATS and zpos
/// are required: planes lacking either, or with a malformed alpha property,
/// are left out of the result. The primary is the first CRTC-compatible
/// Primary plane. Cursor planes are ignored, and planes with an unknown type are
/// warned about and skipped. Planes relying on the fixed-opaque assumption are
/// reported once during discovery. Throws if the CRTC or a usable primary plane
/// cannot be found, or if reading DRM state fails.
auto discover_crtc_plane_capabilities(int drm_fd, uint32_t crtc_id) -> CrtcPlaneCapabilities;
}

#endif // MIR_GRAPHICS_ATOMIC_PLANE_CAPABILITIES_H_
