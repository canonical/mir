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
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
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
constexpr uint32_t other_crtc_mask{0b01};
constexpr uint32_t target_crtc_mask{0b10};
constexpr uint32_t tie_below_primary_id{39};
constexpr uint32_t primary_id{40};
constexpr uint32_t overlay_id{41};
constexpr uint32_t underlay_id{42};
constexpr uint32_t tie_above_primary_id{43};
constexpr uint32_t other_crtc_overlay_id{44};
constexpr uint32_t top_overlay_id{45};
constexpr uint32_t other_primary_id{46};
constexpr uint32_t no_zpos_id{47};
constexpr uint32_t invalid_zpos_id{48};
constexpr uint32_t no_formats_id{49};
constexpr uint32_t fixed_opaque_overlay_id{51};
constexpr uint32_t blend_without_none_id{52};
constexpr uint32_t cursor_id{53};
constexpr uint32_t untyped_id{54};
constexpr uint32_t second_primary_id{56};
constexpr uint32_t inverted_alpha_id{59};
constexpr uint32_t inverted_zpos_id{60};
constexpr uint32_t both_crtcs_mask{target_crtc_mask | other_crtc_mask};
using EnumValues = std::vector<std::pair<uint64_t, std::string>>;

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

constexpr RangeProperty full_alpha{0, 0xffff, 0xffff};
EnumValues const blend_modes{{6, "Pre-multiplied"}, {7, "None"}, {8, "Coverage"}};

struct PlaneSpec
{
    uint32_t id;
    uint32_t crtcs;
    std::optional<uint64_t> type{};
    bool in_formats{true};
    std::optional<RangeProperty> zpos{};
    std::optional<RangeProperty> alpha{};
    EnumValues blend{};
    EnumValues color_encoding{};
    EnumValues color_range{};
};

std::vector<PlaneSpec> const planes{
    {.id = primary_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_PRIMARY, .zpos = fixed_zpos(1),
     .blend = blend_modes},
    {.id = second_primary_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_PRIMARY, .zpos = fixed_zpos(0)},
    {.id = underlay_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY, .zpos = fixed_zpos(0)},
    {.id = tie_below_primary_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY, .zpos = fixed_zpos(1),
     .alpha = full_alpha},
    {.id = tie_above_primary_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY, .zpos = fixed_zpos(1),
     .alpha = full_alpha},
    {.id = fixed_opaque_overlay_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY,
     .zpos = fixed_zpos(2)},
    {.id = blend_without_none_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY, .zpos = fixed_zpos(2),
     .blend = {{6, "Pre-multiplied"}, {8, "Coverage"}}},
    {.id = no_zpos_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY},
    {.id = invalid_zpos_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY,
     .zpos = RangeProperty{2, 2, 2, DRM_MODE_PROP_ENUM}},
    {.id = inverted_zpos_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY,
     .zpos = RangeProperty{4, 0, 2}},
    {.id = no_formats_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY, .in_formats = false,
     .zpos = fixed_zpos(2)},
    {.id = inverted_alpha_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY, .zpos = fixed_zpos(2),
     .alpha = RangeProperty{0xffff, 0, 0xffff}},
    {.id = cursor_id, .crtcs = target_crtc_mask, .type = DRM_PLANE_TYPE_CURSOR, .zpos = fixed_zpos(5)},
    {.id = untyped_id, .crtcs = target_crtc_mask, .zpos = fixed_zpos(2)},
    {.id = overlay_id, .crtcs = both_crtcs_mask, .type = DRM_PLANE_TYPE_OVERLAY, .zpos = fixed_zpos(2),
     .alpha = full_alpha, .blend = blend_modes,
     .color_encoding = {{10, "ITU-R BT.601 YCbCr"}, {11, "ITU-R BT.709 YCbCr"}, {12, "ITU-R BT.2020 YCbCr"}},
     .color_range = {{13, "YCbCr limited range"}, {14, "YCbCr full range"}}},
    {.id = top_overlay_id, .crtcs = both_crtcs_mask, .type = DRM_PLANE_TYPE_OVERLAY, .zpos = RangeProperty{0, 4, 3},
     .blend = blend_modes},
    {.id = other_primary_id, .crtcs = other_crtc_mask, .type = DRM_PLANE_TYPE_PRIMARY, .zpos = fixed_zpos(0)},
    {.id = other_crtc_overlay_id, .crtcs = other_crtc_mask, .type = DRM_PLANE_TYPE_OVERLAY,
     .zpos = RangeProperty{0, 4, 2}},
};

struct InFormatsBlob
{
    drm_format_modifier_blob header{
        FORMAT_BLOB_CURRENT, 0, 3, offsetof(InFormatsBlob, formats), 2, offsetof(InFormatsBlob, modifiers)};
    uint32_t formats[3]{DRM_FORMAT_NV12, DRM_FORMAT_XRGB8888, DRM_FORMAT_ARGB8888};
    drm_format_modifier modifiers[2]{{0b10, 0, 0, I915_FORMAT_MOD_Y_TILED}, {0b01, 2, 0, DRM_FORMAT_MOD_LINEAR}};

    auto bytes() const -> std::span<std::byte const>
    {
        return std::as_bytes(std::span{this, 1});
    }
};

auto ids_of(std::vector<mga::PlaneCapability> const& overlays) -> std::vector<uint32_t>
{
    return overlays | std::views::transform(&mga::PlaneCapability::id) | std::ranges::to<std::vector>();
}

auto find_overlay(mga::CrtcPlaneCapabilities const& caps, uint32_t id) -> mga::PlaneCapability const&
{
    auto const it = std::ranges::find(caps.overlays, id, &mga::PlaneCapability::id);
    if (it == caps.overlays.end())
        throw std::runtime_error("Plane " + std::to_string(id) + " not found");
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
        mir::logging::set_logger(logger);
        mock_drm.reset(drm_device);
        for (auto const id : {other_crtc_id, target_crtc_id})
            mock_drm.add_crtc(drm_device, id, {});
        for (auto const& plane : planes)
            mock_drm.add_plane(drm_device, plane.id, plane.crtcs);
        mock_drm.prepare(drm_device);

        for (auto const& plane : planes)
            add_plane_properties(plane);

        // Discovery warns about the untyped plane; allow it in tests that assert on other log messages.
        EXPECT_CALL(*logger, log(_, _, _)).Times(AnyNumber());
    }

    void TearDown() override
    {
        mir::logging::set_logger(std::make_shared<mtd::NullLogger>());
        ASSERT_TRUE(logger.unique()) << "Test needs to destroy the MockLogger to verify assertions";
        logger.reset();
    }

    auto discover(uint32_t crtc_id = target_crtc_id) const -> mga::CrtcPlaneCapabilities
    {
        return mga::discover_crtc_plane_capabilities(drm_fd, crtc_id);
    }

    auto add_property(uint32_t plane, char const* name, uint64_t value = 0) -> uint32_t
    {
        auto const id = next_property_id++;
        mock_drm.add_property(drm_device, plane, DRM_MODE_OBJECT_PLANE, id, name, value);
        return id;
    }

    auto add_range(uint32_t plane, char const* name, RangeProperty const& range) -> uint32_t
    {
        auto const id = add_property(plane, name, range.value);
        mock_drm.set_property_range(drm_device, id, range.flags, range.min, range.max);
        return id;
    }

    void add_enum(uint32_t plane, char const* name, EnumValues const& values)
    {
        mock_drm.set_property_enums(drm_device, add_property(plane, name), values);
    }

    void add_plane_properties(PlaneSpec const& plane)
    {
        if (plane.type)
            add_property(plane.id, "type", *plane.type);
        if (plane.in_formats)
        {
            add_property(plane.id, "IN_FORMATS", plane.id);
            set_in_formats(plane.id, InFormatsBlob{}.bytes());
        }
        if (plane.zpos)
            add_range(plane.id, "zpos", *plane.zpos);
        if (plane.alpha)
            add_range(plane.id, "alpha", *plane.alpha);
        for (auto const& [name, values] : {std::pair{"pixel blend mode", &plane.blend},
                                          {"COLOR_ENCODING", &plane.color_encoding},
                                          {"COLOR_RANGE", &plane.color_range}})
        {
            if (!values->empty())
                add_enum(plane.id, name, *values);
        }
    }

    void set_in_formats(uint32_t plane, std::span<std::byte const> bytes)
    {
        mock_drm.add_property_blob(drm_device, plane, bytes);
    }

    std::shared_ptr<MockLogger> logger{std::make_shared<MockLogger>()};
    NiceMock<mtd::MockDRM> mock_drm;
    int const drm_fd{mock_drm.open(drm_device, 0)};
    uint32_t next_property_id{1000};
};
}

TEST_F(PlaneCapabilitiesTest, discovers_primary_and_overlays_with_zpos_order_and_stacking)
{
    auto const caps = discover();

    EXPECT_THAT(caps.crtc_id, Eq(target_crtc_id));
    EXPECT_THAT(caps.primary.id, Eq(primary_id));
    EXPECT_THAT(caps.primary.zpos, Eq(mga::ZPos{1, 1, 1, true}));
    auto const stacking = caps.overlays | std::views::transform([](auto const& overlay)
        { return std::tuple{overlay.id, overlay.can_be_above_primary, overlay.can_be_below_primary}; })
        | std::ranges::to<std::vector>();
    ASSERT_THAT(stacking, ElementsAre(
        std::tuple{underlay_id, false, true},
        std::tuple{tie_below_primary_id, false, true},
        std::tuple{tie_above_primary_id, true, false},
        std::tuple{overlay_id, true, false},
        std::tuple{fixed_opaque_overlay_id, true, false},
        std::tuple{blend_without_none_id, true, false},
        std::tuple{top_overlay_id, true, true}))
        << "Rejected planes: no zpos " << no_zpos_id << ", invalid zpos " << invalid_zpos_id
        << ", no IN_FORMATS " << no_formats_id << ", cursor " << cursor_id << ", untyped " << untyped_id
        << ", later primary " << second_primary_id << ", inverted alpha " << inverted_alpha_id
        << ", inverted zpos " << inverted_zpos_id;

    EXPECT_THAT(find_overlay(caps, top_overlay_id).zpos, Eq(mga::ZPos{0, 4, 3, false}));
}

TEST_F(PlaneCapabilitiesTest, discovery_uses_only_planes_routed_to_the_requested_crtc)
{
    auto const other_caps = discover(other_crtc_id);
    EXPECT_THAT(other_caps.crtc_id, Eq(other_crtc_id));
    EXPECT_THAT(other_caps.primary.id, Eq(other_primary_id));
    EXPECT_THAT(ids_of(other_caps.overlays), ElementsAre(overlay_id, other_crtc_overlay_id, top_overlay_id));
}

TEST_F(PlaneCapabilitiesTest, decodes_exact_formats_and_display_properties)
{
    auto const caps = discover();
    auto const& overlay = find_overlay(caps, overlay_id);
    EXPECT_THAT(overlay.formats, UnorderedElementsAre(
        mga::FormatModifier{DRM_FORMAT_XRGB8888, I915_FORMAT_MOD_Y_TILED},
        mga::FormatModifier{DRM_FORMAT_ARGB8888, DRM_FORMAT_MOD_LINEAR}));
    EXPECT_THAT(overlay.alpha_max, Optional(mga::PropertyValue{0xffff}));
    EXPECT_THAT(overlay.blend_none, Optional(mga::PropertyValue{7}));
    EXPECT_FALSE(overlay.fixed_opaque);
    EXPECT_THAT(overlay.bt709_limited, Optional(mga::Bt709Limited{11, 13}));

    auto const& blend_without_none = find_overlay(caps, blend_without_none_id);
    EXPECT_EQ(blend_without_none.blend_none, std::nullopt);
    EXPECT_EQ(blend_without_none.alpha_max, std::nullopt);
    EXPECT_EQ(blend_without_none.bt709_limited, std::nullopt);
    EXPECT_FALSE(blend_without_none.fixed_opaque);
}

namespace
{
struct MalformedInFormats
{
    char const* name;
    std::function<void(InFormatsBlob&)> modify;
};

class MalformedInFormatsTest : public PlaneCapabilitiesTest, public WithParamInterface<MalformedInFormats>
{
};
}

TEST_P(MalformedInFormatsTest, overlay_with_malformed_or_empty_formats_is_not_usable)
{
    InFormatsBlob blob;
    GetParam().modify(blob);
    set_in_formats(overlay_id, blob.bytes());

    EXPECT_THAT(ids_of(discover().overlays), Not(Contains(overlay_id)));
}

INSTANTIATE_TEST_SUITE_P(
    PlaneCapabilities,
    MalformedInFormatsTest,
    Values(
        MalformedInFormats{"ZeroFormatPairs", [](InFormatsBlob& blob) { blob.header.count_modifiers = 0; }},
        MalformedInFormats{"WrongVersion", [](InFormatsBlob& blob) { ++blob.header.version; }},
        MalformedInFormats{"FormatsPastEnd", [](InFormatsBlob& blob)
                           { blob.header.formats_offset = sizeof(InFormatsBlob) - sizeof(uint32_t); }},
        MalformedInFormats{"FormatsInsideHeader", [](InFormatsBlob& blob) { blob.header.formats_offset = 0; }},
        MalformedInFormats{"ModifiersPastEnd", [](InFormatsBlob& blob) { ++blob.header.count_modifiers; }},
        MalformedInFormats{"FormatBitPastEnd", [](InFormatsBlob& blob) { blob.modifiers[0].formats = 0b1000; }}),
    [](TestParamInfo<MalformedInFormats> const& info) { return std::string{info.param.name}; });

TEST_F(PlaneCapabilitiesTest, unusable_first_primary_fails_discovery_despite_later_usable_primary)
{
    set_in_formats(primary_id, {});

    EXPECT_THAT([this] { discover(); }, ThrowsMessage<std::runtime_error>(HasSubstr("No usable primary plane")));
}

TEST_F(PlaneCapabilitiesTest, fixed_opaque_planes_are_reported_once)
{
    EXPECT_CALL(*logger, log(_, AllOf(HasSubstr("fixed-opaque"), EndsWith(": 42, 51")), _)).Times(1);

    discover();
}

TEST_F(PlaneCapabilitiesTest, unreadable_in_formats_blob_fails_discovery)
{
    ON_CALL(mock_drm, drmModeGetPropertyBlob(drm_fd, overlay_id)).WillByDefault(Return(nullptr));

    EXPECT_THAT([this] { discover(); }, ThrowsMessage<std::system_error>(HasSubstr("IN_FORMATS")));
}
