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

#include "plane_capabilities.h"

#include <mir/logging/logger.h>
#include <mir/test/doubles/mock_drm.h>
#include <mir/test/doubles/null_logger.h>

#include <drm_fourcc.h>
#include <drm_mode.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mga = mir::graphics::atomic;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
constexpr char const* drm_device{"/dev/dri/card0"};
constexpr uint32_t other_crtc_id{30};
constexpr uint32_t target_crtc_id{31};
constexpr uint32_t target_crtc_index{1};
constexpr uint32_t other_crtc_mask{0b01};
constexpr uint32_t target_crtc_mask{0b10};
constexpr uint32_t primary_id{40};
constexpr uint32_t overlay_id{41};
constexpr uint32_t underlay_id{42};
constexpr uint32_t other_crtc_overlay_id{44};
constexpr uint32_t top_overlay_id{45};
constexpr uint32_t other_primary_id{46};
constexpr uint32_t no_zpos_id{47};
constexpr uint32_t invalid_zpos_id{48};
constexpr uint32_t no_formats_id{49};
constexpr uint32_t immutable_range_flags{DRM_MODE_PROP_RANGE | DRM_MODE_PROP_IMMUTABLE};
using EnumValues = std::vector<std::pair<uint64_t, std::string>>;

struct InFormatsBlob
{
    drm_format_modifier_blob header{
        FORMAT_BLOB_CURRENT, 0, 2, offsetof(InFormatsBlob, formats), 2, offsetof(InFormatsBlob, modifiers)};
    uint32_t formats[2]{DRM_FORMAT_NV12, DRM_FORMAT_XRGB8888};
    drm_format_modifier modifiers[2]{{0b01, 0, 0, I915_FORMAT_MOD_Y_TILED}, {0b10, 0, 0, DRM_FORMAT_MOD_LINEAR}};

    auto bytes() const -> std::span<std::byte const>
    {
        return std::as_bytes(std::span{this, 1});
    }
};

auto ids_of(std::vector<mga::PlaneCapabilities> const& overlays) -> std::vector<uint32_t>
{
    return overlays | std::views::transform(&mga::PlaneCapabilities::id) | std::ranges::to<std::vector>();
}

auto find_overlay(mga::CrtcPlaneCapabilities const& caps, uint32_t id) -> std::optional<mga::PlaneCapabilities>
{
    auto const it = std::ranges::find(caps.overlays, id, &mga::PlaneCapabilities::id);
    if (it == caps.overlays.end())
        return std::nullopt;
    return *it;
}

class MockLogger : public mir::logging::Logger
{
public:
    MOCK_METHOD(void, log, (mir::logging::Severity severity, std::string const& message,
                            std::string const& component), (override));
};

class PlaneCapabilitiesTest : public Test
{
public:
    PlaneCapabilitiesTest()
    {
        mock_drm.reset(drm_device);
        for (auto const id : {other_crtc_id, target_crtc_id})
            mock_drm.add_crtc(drm_device, id, {});
        for (auto const id : {primary_id, underlay_id, no_zpos_id, invalid_zpos_id, no_formats_id})
            mock_drm.add_plane(drm_device, id, target_crtc_mask);
        for (auto const id : {overlay_id, top_overlay_id})
            mock_drm.add_plane(drm_device, id, target_crtc_mask | other_crtc_mask);
        for (auto const id : {other_primary_id, other_crtc_overlay_id})
            mock_drm.add_plane(drm_device, id, other_crtc_mask);
        mock_drm.prepare(drm_device);

        for (auto const id : {primary_id, other_primary_id})
            add_plane_properties(id, DRM_PLANE_TYPE_PRIMARY);
        for (auto const id : {overlay_id, underlay_id, top_overlay_id, other_crtc_overlay_id,
                              no_zpos_id, invalid_zpos_id})
            add_plane_properties(id, DRM_PLANE_TYPE_OVERLAY);
        add_property(no_formats_id, "type", DRM_PLANE_TYPE_OVERLAY);
        for (auto const& [id, zpos] : {std::pair{primary_id, 1}, {overlay_id, 2}, {underlay_id, 0}, {no_formats_id, 2}})
            add_range(id, "zpos", zpos, zpos, zpos, immutable_range_flags);
        for (auto const& [id, zpos] : {std::pair{top_overlay_id, 3}, {other_crtc_overlay_id, 2}})
            add_range(id, "zpos", 0, 4, zpos);
        add_range(invalid_zpos_id, "zpos", 2, 2, 2, DRM_MODE_PROP_ENUM);
        for (auto const id : {primary_id, overlay_id, other_crtc_overlay_id, underlay_id})
            add_range(id, "alpha", 0, 0xffff, 0xffff);
        for (auto const id : {primary_id, overlay_id, top_overlay_id})
            add_enum(id, "pixel blend mode", {{6, "Pre-multiplied"}, {7, "None"}, {8, "Coverage"}});
        add_enum(overlay_id, "COLOR_ENCODING",
            {{10, "ITU-R BT.601 YCbCr"}, {11, "ITU-R BT.709 YCbCr"}, {12, "ITU-R BT.2020 YCbCr"}});
        add_enum(overlay_id, "COLOR_RANGE", {{13, "YCbCr limited range"}, {14, "YCbCr full range"}});
    }

    void TearDown() override
    {
        mir::logging::set_logger(std::make_shared<mtd::NullLogger>());
    }

    auto discover(uint32_t crtc_id = target_crtc_id) const -> mga::CrtcPlaneCapabilities
    {
        return mga::discover_plane_capabilities(drm_fd, crtc_id);
    }

    auto add_property(uint32_t plane, char const* name, uint64_t value = 0) -> uint32_t
    {
        auto const id = next_property_id++;
        mock_drm.add_property(drm_device, plane, DRM_MODE_OBJECT_PLANE, id, name, value);
        return id;
    }

    void add_range(uint32_t plane, char const* name, uint64_t min, uint64_t max, uint64_t value,
                   uint32_t flags = DRM_MODE_PROP_RANGE)
    {
        mock_drm.set_property_range(drm_device, add_property(plane, name, value), flags, min, max);
    }

    void add_enum(uint32_t plane, char const* name, EnumValues const& values)
    {
        mock_drm.set_property_enums(drm_device, add_property(plane, name), values);
    }

    void add_plane_properties(uint32_t plane, uint64_t type)
    {
        add_property(plane, "type", type);
        add_property(plane, "IN_FORMATS", plane);
        mock_drm.add_property_blob(drm_device, plane, InFormatsBlob{}.bytes());
    }

    NiceMock<mtd::MockDRM> mock_drm;
    int const drm_fd{mock_drm.open(drm_device, 0)};
    uint32_t next_property_id{1000};
};

struct MalformedInFormats
{
    std::string_view name;
    std::function<void(InFormatsBlob&)> modify;
    size_t length{sizeof(InFormatsBlob)};
};

class MalformedInFormatsTest : public PlaneCapabilitiesTest, public WithParamInterface<MalformedInFormats>
{
};
}

TEST_F(PlaneCapabilitiesTest, discovers_routing_types_and_order_relative_to_primary)
{
    auto const caps = discover();

    EXPECT_THAT(caps.crtc_id, Eq(target_crtc_id));
    EXPECT_THAT(caps.crtc_index, Eq(target_crtc_index));
    EXPECT_THAT(caps.primary.id, Eq(primary_id));
    EXPECT_THAT(caps.primary.zpos, Optional(mga::ZPos{1, 1, 1, true}));
    ASSERT_THAT(ids_of(caps.overlays), ElementsAre(underlay_id, overlay_id, top_overlay_id))
        << "Rejected planes: missing zpos " << no_zpos_id << ", invalid zpos " << invalid_zpos_id
        << ", missing IN_FORMATS " << no_formats_id;

    auto const& underlay = caps.overlays[0];
    auto const& overlay = caps.overlays[1];
    auto const& top = caps.overlays[2];
    EXPECT_FALSE(underlay.can_be_above_primary);
    EXPECT_TRUE(underlay.can_be_below_primary);
    EXPECT_TRUE(overlay.can_be_above_primary);
    EXPECT_FALSE(overlay.can_be_below_primary);
    EXPECT_TRUE(top.can_be_above_primary);
    EXPECT_TRUE(top.can_be_below_primary);
    EXPECT_THAT(top.zpos, Optional(mga::ZPos{0, 4, 3, false}));
}

TEST_F(PlaneCapabilitiesTest, decodes_exact_formats_and_display_properties)
{
    auto const overlay = find_overlay(discover(), overlay_id);
    ASSERT_TRUE(overlay.has_value());
    EXPECT_THAT(overlay->formats, UnorderedElementsAre(
        mga::FormatModifier{DRM_FORMAT_NV12, I915_FORMAT_MOD_Y_TILED},
        mga::FormatModifier{DRM_FORMAT_XRGB8888, DRM_FORMAT_MOD_LINEAR}));
    EXPECT_THAT(overlay->formats, Not(Contains(mga::FormatModifier{DRM_FORMAT_NV12, DRM_FORMAT_MOD_LINEAR})));
    EXPECT_THAT(overlay->alpha_max, Optional(mga::PropertyValue{0xffff}));
    EXPECT_THAT(overlay->blend_none, Optional(mga::PropertyValue{7}));
    EXPECT_FALSE(overlay->fixed_opaque);
    EXPECT_THAT(overlay->bt709_limited, Optional(mga::Bt709Limited{11, 13}));
}

TEST_F(PlaneCapabilitiesTest, nonzero_modifier_offset_selects_the_right_format)
{
    InFormatsBlob blob;
    blob.modifiers[1].offset = 1;
    blob.modifiers[1].formats = 1;
    mock_drm.add_property_blob(drm_device, overlay_id, blob.bytes());

    auto const overlay = find_overlay(discover(), overlay_id);
    ASSERT_TRUE(overlay.has_value());
    EXPECT_THAT(overlay->formats, UnorderedElementsAre(
        mga::FormatModifier{DRM_FORMAT_NV12, I915_FORMAT_MOD_Y_TILED},
        mga::FormatModifier{DRM_FORMAT_XRGB8888, DRM_FORMAT_MOD_LINEAR}));
}

TEST_P(MalformedInFormatsTest, overlay_with_malformed_or_empty_formats_is_not_usable)
{
    InFormatsBlob blob;
    GetParam().modify(blob);
    mock_drm.add_property_blob(drm_device, overlay_id, blob.bytes().first(GetParam().length));

    EXPECT_THAT(ids_of(discover().overlays), ElementsAre(underlay_id, top_overlay_id));
}

INSTANTIATE_TEST_SUITE_P(
    PlaneCapabilities,
    MalformedInFormatsTest,
    Values(
        MalformedInFormats{"ZeroFormatPairs", [](InFormatsBlob& blob) { blob.header.count_modifiers = 0; }},
        MalformedInFormats{"WrongVersion", [](InFormatsBlob& blob) { ++blob.header.version; }},
        MalformedInFormats{"FormatsPastEnd", [](InFormatsBlob& blob)
                           { blob.header.formats_offset = sizeof(InFormatsBlob) - sizeof(uint32_t); }},
        MalformedInFormats{"ModifiersPastEnd", [](InFormatsBlob&) {},
                           sizeof(InFormatsBlob) - sizeof(drm_format_modifier)},
        MalformedInFormats{"FormatBitPastEnd", [](InFormatsBlob& blob) { blob.modifiers[0].formats = 0b100; }}),
    [](TestParamInfo<MalformedInFormats> const& info) { return std::string{info.param.name}; });

TEST_F(PlaneCapabilitiesTest, primary_without_zpos_is_below_every_overlay)
{
    auto const caps = discover(other_crtc_id);

    EXPECT_THAT(caps.primary.id, Eq(other_primary_id));
    EXPECT_FALSE(caps.primary.zpos.has_value());
    ASSERT_THAT(ids_of(caps.overlays), ElementsAre(overlay_id, other_crtc_overlay_id, top_overlay_id));
    for (auto const& overlay : caps.overlays)
    {
        EXPECT_TRUE(overlay.can_be_above_primary) << "overlay " << overlay.id;
        EXPECT_FALSE(overlay.can_be_below_primary) << "overlay " << overlay.id;
    }
}

TEST_F(PlaneCapabilitiesTest, missing_alpha_or_blend_assumes_fixed_opaque_reported_once_and_color_is_not_assumed)
{
    auto const logger = std::make_shared<NiceMock<MockLogger>>();
    mir::logging::set_logger(logger);
    auto const expected_ids = std::to_string(other_primary_id) + ", " + std::to_string(other_crtc_overlay_id) +
        ", " + std::to_string(top_overlay_id);
    EXPECT_CALL(*logger, log(_, AllOf(HasSubstr("fixed-opaque"), HasSubstr(expected_ids)), _)).Times(1);

    auto const caps = discover(other_crtc_id);
    auto const alpha_only = find_overlay(caps, other_crtc_overlay_id);
    auto const blend_only = find_overlay(caps, top_overlay_id);
    ASSERT_TRUE(alpha_only.has_value());
    ASSERT_TRUE(blend_only.has_value());
    EXPECT_TRUE(alpha_only->fixed_opaque);
    EXPECT_TRUE(blend_only->fixed_opaque);
    EXPECT_TRUE(caps.primary.fixed_opaque);
    EXPECT_THAT(alpha_only->alpha_max, Optional(mga::PropertyValue{0xffff}));
    EXPECT_FALSE(alpha_only->blend_none.has_value());
    EXPECT_FALSE(blend_only->alpha_max.has_value());
    EXPECT_THAT(blend_only->blend_none, Optional(mga::PropertyValue{7}));
    EXPECT_FALSE(caps.primary.alpha_max.has_value());
    EXPECT_FALSE(caps.primary.blend_none.has_value());
    EXPECT_FALSE(alpha_only->bt709_limited.has_value());
    EXPECT_FALSE(blend_only->bt709_limited.has_value());
    EXPECT_FALSE(caps.primary.bt709_limited.has_value());
}

TEST_F(PlaneCapabilitiesTest, rebuild_replaces_state_as_one_unit)
{
    mga::PlaneCapabilityState state;
    state.rebuild(drm_fd, target_crtc_id);
    auto const first = state.current();
    ASSERT_NE(first, nullptr);
    EXPECT_THAT(ids_of(first->overlays), Contains(overlay_id));

    mock_drm.add_property_blob(drm_device, overlay_id, {});
    state.rebuild(drm_fd, target_crtc_id);
    auto const second = state.current();

    ASSERT_NE(second, nullptr);
    EXPECT_THAT(ids_of(second->overlays), Not(Contains(overlay_id)));
    EXPECT_THAT(ids_of(first->overlays), Contains(overlay_id));
    EXPECT_NE(first.get(), second.get());
}

TEST_F(PlaneCapabilitiesTest, failed_rebuild_leaves_no_capabilities)
{
    mga::PlaneCapabilityState state;
    state.rebuild(drm_fd, target_crtc_id);
    ASSERT_NE(state.current(), nullptr);

    mock_drm.add_property_blob(drm_device, primary_id, {});
    EXPECT_NO_THROW(state.rebuild(drm_fd, target_crtc_id));
    EXPECT_EQ(state.current(), nullptr);
}
