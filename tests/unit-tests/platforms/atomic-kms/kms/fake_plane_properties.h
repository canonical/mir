/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_TEST_FAKE_PLANE_PROPERTIES_H_
#define MIR_TEST_FAKE_PLANE_PROPERTIES_H_

#include <mir/test/doubles/mock_drm.h>

#include <drm_fourcc.h>
#include <drm_mode.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace mir
{
namespace test
{
using EnumValues = std::span<std::pair<uint64_t, std::string> const>;

struct RangeProperty
{
    uint64_t min;
    uint64_t max;
    uint64_t value;
    uint32_t flags{DRM_MODE_PROP_RANGE};
};

constexpr auto fixed_zpos(uint64_t zpos) -> RangeProperty
{
    return {zpos, zpos, zpos, DRM_MODE_PROP_RANGE | DRM_MODE_PROP_IMMUTABLE};
}

// KMS plane "alpha" uses 16-bit unsigned opacity (0xffff = DRM_BLEND_ALPHA_OPAQUE).
constexpr RangeProperty full_alpha{0, 0xffff, 0xffff};

// Arbitrary distinct non-zero enum values (kernel ordinals start at 0) so tests verify
// that KMS enum properties are resolved by name and not confused with one another.
namespace blend { constexpr uint64_t premultiplied{6}, none{7}, coverage{8}; }
namespace color_encoding { constexpr uint64_t bt601{10}, bt709{11}, bt2020{12}; }
namespace color_range { constexpr uint64_t limited{13}, full{14}; }

inline std::pair<uint64_t, std::string> const blend_modes[]{
    {blend::premultiplied, "Pre-multiplied"}, {blend::none, "None"}, {blend::coverage, "Coverage"}};
inline std::pair<uint64_t, std::string> const color_encodings[]{
    {color_encoding::bt601, "ITU-R BT.601 YCbCr"},
    {color_encoding::bt709, "ITU-R BT.709 YCbCr"},
    {color_encoding::bt2020, "ITU-R BT.2020 YCbCr"}};
inline std::pair<uint64_t, std::string> const color_ranges[]{
    {color_range::limited, "YCbCr limited range"}, {color_range::full, "YCbCr full range"}};

/// A fake KMS plane and the properties it advertises; absent optionals and empty enums are not added
struct PlaneSpec
{
    uint32_t id;
    uint32_t crtcs{0};
    std::optional<uint64_t> type{};
    bool in_formats{true};
    uint32_t bound_crtc{0};
    std::optional<RangeProperty> zpos{};
    std::optional<RangeProperty> alpha{};
    EnumValues blend{};
    EnumValues color_encoding{};
    EnumValues color_range{};
};

/// An IN_FORMATS blob advertising one linear format
struct SingleFormatBlob
{
    explicit SingleFormatBlob(uint32_t drm_format)
        : format{drm_format}
    {
    }

    drm_format_modifier_blob header{
        FORMAT_BLOB_CURRENT, 0, 1, offsetof(SingleFormatBlob, format), 1, offsetof(SingleFormatBlob, modifier)};
    uint32_t format;
    drm_format_modifier modifier{1, 0, 0, DRM_FORMAT_MOD_LINEAR};

    auto bytes() const -> std::span<std::byte const>
    {
        return std::as_bytes(std::span{this, 1});
    }
};

/// A plane's property IDs, derived from its plane ID so that tests can name them without a fixture lookup.
/// Planes are spaced far enough apart that their property IDs neither overlap nor reach small object IDs.
struct PlanePropertyIds
{
    uint32_t plane;
    uint32_t type{plane * 0x100};
    uint32_t src_x{type + 1}, src_y{src_x + 1}, src_w{src_y + 1}, src_h{src_w + 1};
    uint32_t crtc_x{src_h + 1}, crtc_y{crtc_x + 1}, crtc_w{crtc_y + 1}, crtc_h{crtc_w + 1};
    uint32_t crtc_id{crtc_h + 1}, fb_id{crtc_id + 1}, in_formats{fb_id + 1};
    uint32_t zpos{in_formats + 1}, alpha{zpos + 1}, pixel_blend_mode{alpha + 1};
    uint32_t color_encoding{pixel_blend_mode + 1}, color_range{color_encoding + 1};
};

/// Adds planes and their properties to a MockDRM device, with the IDs given by PlanePropertyIds.
/// Call MockDRM::prepare() after adding planes. A plane's IN_FORMATS property points at a blob
/// whose ID is the plane ID.
class FakePlaneProperties
{
public:
    FakePlaneProperties(doubles::MockDRM& mock_drm, std::string device_name)
        : drm{mock_drm},
          device{std::move(device_name)}
    {
    }

    void add(PlaneSpec const& plane, std::span<std::byte const> in_formats_blob)
    {
        drm.add_plane(device.c_str(), plane.id, plane.crtcs);
        PlanePropertyIds const ids{plane.id};
        if (plane.type)
            add_property(plane.id, ids.type, "type", *plane.type);
        for (auto const& [id, name] : {std::pair{ids.src_x, "SRC_X"}, {ids.src_y, "SRC_Y"},
                                       {ids.src_w, "SRC_W"}, {ids.src_h, "SRC_H"},
                                       {ids.crtc_x, "CRTC_X"}, {ids.crtc_y, "CRTC_Y"},
                                       {ids.crtc_w, "CRTC_W"}, {ids.crtc_h, "CRTC_H"},
                                       {ids.crtc_id, "CRTC_ID"}, {ids.fb_id, "FB_ID"}})
            add_property(plane.id, id, name, id == ids.crtc_id ? plane.bound_crtc : 0);
        if (plane.in_formats)
        {
            add_property(plane.id, ids.in_formats, "IN_FORMATS", plane.id);
            set_in_formats(plane.id, in_formats_blob);
        }
        if (plane.zpos)
            add_range(plane.id, ids.zpos, "zpos", *plane.zpos);
        if (plane.alpha)
            add_range(plane.id, ids.alpha, "alpha", *plane.alpha);
        if (!plane.blend.empty())
            add_enum(plane.id, ids.pixel_blend_mode, "pixel blend mode", plane.blend);
        if (!plane.color_encoding.empty())
            add_enum(plane.id, ids.color_encoding, "COLOR_ENCODING", plane.color_encoding);
        if (!plane.color_range.empty())
            add_enum(plane.id, ids.color_range, "COLOR_RANGE", plane.color_range);
    }

    void set_in_formats(uint32_t plane, std::span<std::byte const> bytes)
    {
        drm.add_property_blob(device.c_str(), plane, bytes);
    }

private:
    void add_property(uint32_t plane, uint32_t id, char const* name, uint64_t value = 0)
    {
        drm.add_property(device.c_str(), plane, DRM_MODE_OBJECT_PLANE, id, name, value);
    }

    void add_range(uint32_t plane, uint32_t id, char const* name, RangeProperty const& range)
    {
        add_property(plane, id, name, range.value);
        drm.set_property_range(device.c_str(), id, range.flags, range.min, range.max);
    }

    void add_enum(uint32_t plane, uint32_t id, char const* name, EnumValues const& values)
    {
        add_property(plane, id, name);
        drm.set_property_enums(device.c_str(), id, values);
    }

    doubles::MockDRM& drm;
    std::string const device;
};
}
}

#endif // MIR_TEST_FAKE_PLANE_PROPERTIES_H_
