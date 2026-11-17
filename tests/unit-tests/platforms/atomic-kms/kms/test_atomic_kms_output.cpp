/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "atomic_kms_output.h"
#include "fake_plane_properties.h"
#include "plane_capabilities.h"
#include "presentation_state.h"

#include <mir/geometry/rectangle.h>
#include <mir/graphics/kms/drm_mode_resources.h>
#include <mir/graphics/kms_framebuffer.h>
#include <mir/graphics/gamma_curves.h>
#include <mir/test/doubles/mock_drm.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <drm_fourcc.h>

#include <cerrno>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mga = mir::graphics::atomic;
namespace mg = mir::graphics;
namespace mgk = mir::graphics::kms;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
constexpr char const* drm_device{"/dev/dri/card0"};
constexpr uint32_t no_flags{0};
constexpr int mode_width{1920};
constexpr int mode_height{1080};
geom::Size const requested_mode_size{mode_width, mode_height};
geom::Size const smaller_crtc_size{1280, 720};
geom::Displacement const source_offset{17, 29};
mg::GammaCurves const sample_gamma{{1}, {2}, {3}};

auto kms_fixed_point(uint32_t value) -> uint64_t
{
    constexpr int fractional_bits{16};
    return static_cast<uint64_t>(value) << fractional_bits;
}

class StubFramebuffer : public mg::FBHandle
{
public:
    explicit StubFramebuffer(uint32_t id, geom::Size size = requested_mode_size) : id_{id}, size_{size} {}

    operator uint32_t() const override
    {
        return id_;
    }

    auto size() const -> geom::Size override
    {
        return size_;
    }

private:
    uint32_t const id_;
    geom::Size const size_;
};

using AtomicPropertyKey = mtd::AtomicPropertyKey;

struct PrimaryRequestParameters
{
    uint32_t framebuffer_id;
    geom::Displacement source_offset{};
    bool activate_crtc{false};
};

class AtomicKMSOutputTest : public Test
{
public:
    static constexpr uint32_t connector_crtc_property_id{101};
    static constexpr uint32_t crtc_active_property_id{201};
    static constexpr uint32_t crtc_mode_property_id{202};
    static constexpr uint32_t crtc_gamma_lut_property_id{203};
    static constexpr uint32_t mode_blob_id{1};
    static constexpr uint32_t gamma_lut_blob_id{2};
    static constexpr uint32_t primary_framebuffer_id{77};
    static constexpr uint32_t flip_framebuffer_id{88};
    static constexpr uint32_t connector_id{30};
    static constexpr uint32_t encoder_id{20};
    static constexpr uint32_t crtc_id{10};
    static constexpr uint32_t plane_id{40};
    // FakePlaneProperties uses the plane ID as its IN_FORMATS blob ID
    static constexpr uint32_t plane_formats_blob_id{plane_id};
    static constexpr mt::PlanePropertyIds primary_properties{plane_id};
    static constexpr uint64_t primary_zpos{0};
    static constexpr uint32_t primary_crtc_mask{1};

    AtomicKMSOutputTest()
        : AtomicKMSOutputTest{true}
    {
    }

    explicit AtomicKMSOutputTest(bool with_gamma_lut)
        : drm_fd{mock_drm.open(drm_device, 0)}
    {
        setup_drm_objects(with_gamma_lut);
    }

    auto output(geom::Displacement offset = {}) -> std::unique_ptr<mga::AtomicKMSOutput>
    {
        // Only the outer copy is owned here; its mode/encoder arrays belong to the fixture.
        mgk::DRMModeConnectorUPtr connector_copy{
            nullptr, std::default_delete<drmModeConnector>{}};
        connector_copy.reset(new drmModeConnector{connector()});
        auto result = std::make_unique<mga::AtomicKMSOutput>(
            std::move(drm_fd), std::move(connector_copy));
        result->configure(offset, 0);
        return result;
    }

    auto disabled_properties() const -> std::map<AtomicPropertyKey, uint64_t>
    {
        return {
            {{connector_id, connector_crtc_property_id}, 0},
            {{crtc_id, crtc_active_property_id}, 0},
            {{crtc_id, crtc_mode_property_id}, 0},
            {{plane_id, primary_properties.fb_id}, 0},
            {{plane_id, primary_properties.crtc_id}, 0}};
    }

    auto scanout_request_properties(
        uint32_t plane,
        uint32_t framebuffer_id,
        geom::Displacement source_offset,
        geom::Rectangle destination) const -> std::map<AtomicPropertyKey, uint64_t>
    {
        mt::PlanePropertyIds const ids{plane};
        auto const width = destination.size.width.as_uint32_t();
        auto const height = destination.size.height.as_uint32_t();
        return {
            {{plane, ids.fb_id}, framebuffer_id},
            {{plane, ids.crtc_id}, crtc_id},
            {{plane, ids.src_x}, kms_fixed_point(source_offset.dx.as_uint32_t())},
            {{plane, ids.src_y}, kms_fixed_point(source_offset.dy.as_uint32_t())},
            {{plane, ids.src_w}, kms_fixed_point(width)},
            {{plane, ids.src_h}, kms_fixed_point(height)},
            {{plane, ids.crtc_x}, static_cast<uint64_t>(static_cast<int64_t>(destination.top_left.x.as_int()))},
            {{plane, ids.crtc_y}, static_cast<uint64_t>(static_cast<int64_t>(destination.top_left.y.as_int()))},
            {{plane, ids.crtc_w}, width},
            {{plane, ids.crtc_h}, height}};
    }

    auto primary_request_properties(PrimaryRequestParameters const& parameters) const
        -> std::map<AtomicPropertyKey, uint64_t>
    {
        std::map<AtomicPropertyKey, uint64_t> properties{
            {{crtc_id, crtc_mode_property_id}, mode_blob_id}, {{connector_id, connector_crtc_property_id}, crtc_id}};
        if (parameters.activate_crtc)
            properties[{crtc_id, crtc_active_property_id}] = 1;
        properties.merge(scanout_request_properties(
            plane_id, parameters.framebuffer_id, parameters.source_offset, {{}, requested_mode_size}));
        return properties;
    }

    auto crtc() -> drmModeCrtc&
    {
        return *mock_drm.find_crtc(drm_device, crtc_id);
    }

    auto connector() -> drmModeConnector&
    {
        return *mock_drm.find_connector(drm_device, connector_id);
    }

    NiceMock<mtd::MockDRM> mock_drm;
    mir::Fd drm_fd;
    mt::FakePlaneProperties plane_properties{mock_drm, drm_device};

private:
    void setup_drm_objects(bool with_gamma_lut)
    {
        constexpr uint16_t horizontal_total{2000};
        constexpr uint16_t vertical_total{1100};
        constexpr uint32_t pixel_clock_khz{148500};
        drmModeModeInfo mode{};
        mode.hdisplay = mode_width;
        mode.vdisplay = mode_height;
        mode.htotal = horizontal_total;
        mode.vtotal = vertical_total;
        mode.clock = pixel_clock_khz;
        modes.push_back(mode);

        mock_drm.reset(drm_device);
        mock_drm.add_crtc(drm_device, crtc_id, mode);
        crtc().width = mode_width;
        crtc().height = mode_height;
        mock_drm.add_encoder(drm_device, encoder_id, crtc_id, primary_crtc_mask);
        mock_drm.add_connector(
            drm_device, connector_id, DRM_MODE_CONNECTOR_VGA, DRM_MODE_CONNECTED, encoder_id, modes, encoder_ids, {});
        connector().connector_type_id = 1;
        plane_properties.add(
            {.id = plane_id, .crtcs = primary_crtc_mask, .type = DRM_PLANE_TYPE_PRIMARY,
             .zpos = mt::fixed_zpos(primary_zpos)},
            mt::SingleFormatBlob{DRM_FORMAT_XRGB8888}.bytes());
        mock_drm.prepare(drm_device);

        mock_drm.add_property(
            drm_device, connector_id, DRM_MODE_OBJECT_CONNECTOR, connector_crtc_property_id, "CRTC_ID");
        mock_drm.add_property(drm_device, crtc_id, DRM_MODE_OBJECT_CRTC, crtc_active_property_id, "ACTIVE");
        mock_drm.add_property(drm_device, crtc_id, DRM_MODE_OBJECT_CRTC, crtc_mode_property_id, "MODE_ID");
        if (with_gamma_lut)
        {
            mock_drm.add_property(drm_device, crtc_id, DRM_MODE_OBJECT_CRTC, crtc_gamma_lut_property_id, "GAMMA_LUT");
        }
    }

    std::vector<drmModeModeInfo> modes;
    std::vector<uint32_t> encoder_ids{encoder_id};
};

struct FramebufferMismatch
{
    std::string_view name;
    geom::Size size;
};

class AtomicKMSFramebufferSizeTest :
    public AtomicKMSOutputTest, public WithParamInterface<FramebufferMismatch>
{
};

struct DisableCase
{
    std::string_view name;
    void (mga::AtomicKMSOutput::*operation)();
    bool disconnect_connector{false};
};

class AtomicKMSDisableTest : public AtomicKMSOutputTest, public WithParamInterface<DisableCase>
{
};

class AtomicKMSOutputWithoutGammaLutTest : public AtomicKMSOutputTest
{
public:
    AtomicKMSOutputWithoutGammaLutTest()
        : AtomicKMSOutputTest{false}
    {
    }
};

// Partly off-screen so mixed-request cases also verify signed CRTC_X/CRTC_Y encoding
geom::Point const client_destination{-40, -20};

struct OverlayPlane
{
    uint32_t id;
    uint64_t zpos_min;
};

struct MixedRequestCase
{
    std::string_view name;
    OverlayPlane overlay;
    uint64_t expected_overlay_zpos;
};

/// An overlay that every mixed state below can legally use
OverlayPlane const legal_overlay{
    .id = AtomicKMSOutputTest::plane_id + 1,
    .zpos_min = 0};

/// The fixture can add one overlay plane (via add_overlay_plane) that can be routed to the CRTC
/// and scan out BT.709 limited-range YCbCr, so that mixed presentations are testable
class AtomicKMSMixedPresentationTest : public AtomicKMSOutputTest
{
public:
    static constexpr uint64_t overlay_zpos_max{3};

    static constexpr uint32_t client_framebuffer_id{99};
    static constexpr geom::Size client_framebuffer_size{640, 480};

    void add_overlay_plane(
        OverlayPlane const& overlay,
        mt::EnumValues const& color_encodings = mt::color_encodings)
    {
        plane_properties.add(
            {.id = overlay.id,
             .crtcs = primary_crtc_mask,
             .type = DRM_PLANE_TYPE_OVERLAY,
             .zpos = mt::RangeProperty{overlay.zpos_min, overlay_zpos_max, overlay_zpos_max},
             .alpha = mt::full_alpha,
             .blend = mt::blend_modes,
             .color_encoding = color_encodings,
             .color_range = mt::color_ranges},
            mt::SingleFormatBlob{DRM_FORMAT_NV12}.bytes());
        // prepare() rebuilds the plane list, so re-run it now that the overlay exists
        mock_drm.prepare(drm_device);
    }

    auto mixed_state(
        uint32_t plane = plane_id + 1,
        geom::Size primary_size = requested_mode_size) const -> mga::PresentationState
    {
        return mga::PresentationState::mixed(
            std::make_shared<StubFramebuffer>(primary_framebuffer_id, primary_size),
            {mga::ClientPlaneAssignment{
                plane,
                std::make_shared<StubFramebuffer>(client_framebuffer_id, client_framebuffer_size),
                client_destination}});
    }

    auto mixed_request_properties(MixedRequestCase const& request) const -> std::map<AtomicPropertyKey, uint64_t>
    {
        mt::PlanePropertyIds const ids{request.overlay.id};
        auto properties = primary_request_properties({.framebuffer_id = primary_framebuffer_id});
        properties.merge(scanout_request_properties(
            request.overlay.id, client_framebuffer_id, {}, {client_destination, client_framebuffer_size}));
        properties.insert(
            {{{request.overlay.id, ids.alpha}, mt::full_alpha.max},
             {{request.overlay.id, ids.pixel_blend_mode}, mt::blend::none},
             {{request.overlay.id, ids.color_encoding}, mt::color_encoding::bt709},
             {{request.overlay.id, ids.color_range}, mt::color_range::limited},
             {{request.overlay.id, ids.zpos}, request.expected_overlay_zpos}});
        return properties;
    }

    auto with_overlay_disabled(std::map<AtomicPropertyKey, uint64_t> properties) const
        -> std::map<AtomicPropertyKey, uint64_t>
    {
        mt::PlanePropertyIds const ids{legal_overlay.id};
        properties[{legal_overlay.id, ids.fb_id}] = 0;
        properties[{legal_overlay.id, ids.crtc_id}] = 0;
        return properties;
    }
};

class AtomicKMSMixedRequestTest : public AtomicKMSMixedPresentationTest, public WithParamInterface<MixedRequestCase>
{
};

struct MixedPresentationRejectionCase
{
    std::string_view name;
    geom::Size primary_size;
    uint32_t assignment_plane;
    bool bt709_limited;
    bool reset_output;
};

class AtomicKMSMixedRejectionTest : public AtomicKMSMixedPresentationTest,
                                    public WithParamInterface<MixedPresentationRejectionCase>
{
};

struct FollowUpCase
{
    std::string_view name;
    std::function<void(AtomicKMSMixedPresentationTest const&, mga::AtomicKMSOutput&)> follow_up;
    std::map<AtomicPropertyKey, uint64_t> (*expected)(AtomicKMSMixedPresentationTest const&);
    uint32_t flags;
};

class AtomicKMSMixedFollowUpTest :
    public AtomicKMSMixedPresentationTest,
    public WithParamInterface<FollowUpCase>
{
};

void follow_up_with_full_scene_page_flip(AtomicKMSMixedPresentationTest const&, mga::AtomicKMSOutput& output)
{
    output.page_flip(mga::PresentationState::full_scene(
        std::make_shared<StubFramebuffer>(AtomicKMSOutputTest::flip_framebuffer_id)));
}

void follow_up_with_set_crtc(AtomicKMSMixedPresentationTest const&, mga::AtomicKMSOutput& output)
{
    output.set_crtc(StubFramebuffer{AtomicKMSOutputTest::primary_framebuffer_id});
}

void follow_up_with_mixed_page_flip(AtomicKMSMixedPresentationTest const& fixture, mga::AtomicKMSOutput& output)
{
    output.page_flip(fixture.mixed_state(legal_overlay.id));
}

auto page_flip_disabling_the_overlay(AtomicKMSMixedPresentationTest const& fixture)
    -> std::map<AtomicPropertyKey, uint64_t>
{
    return fixture.with_overlay_disabled(
        fixture.primary_request_properties({.framebuffer_id = AtomicKMSOutputTest::flip_framebuffer_id}));
}

auto page_flip_keeping_the_overlay(AtomicKMSMixedPresentationTest const& fixture)
    -> std::map<AtomicPropertyKey, uint64_t>
{
    return fixture.mixed_request_properties(MixedRequestCase{
        .name = "higher_id_overlay_shares_primary_zpos",
        .overlay = legal_overlay,
        .expected_overlay_zpos = AtomicKMSOutputTest::primary_zpos});
}

auto set_crtc_disabling_the_overlay(AtomicKMSMixedPresentationTest const& fixture)
    -> std::map<AtomicPropertyKey, uint64_t>
{
    return fixture.with_overlay_disabled(fixture.primary_request_properties(
        {.framebuffer_id = AtomicKMSOutputTest::primary_framebuffer_id, .activate_crtc = true}));
}
}

TEST_F(AtomicKMSOutputTest, set_crtc_preserves_primary_request_and_modeset_flag)
{
    auto output_under_test = output(source_offset);
    auto const properties = primary_request_properties(
        {.framebuffer_id = primary_framebuffer_id, .source_offset = source_offset, .activate_crtc = true});
    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device), mtd::AtomicRequestWith(Eq(properties)), DRM_MODE_ATOMIC_ALLOW_MODESET));

    StubFramebuffer framebuffer{primary_framebuffer_id};
    EXPECT_TRUE(output_under_test->set_crtc(framebuffer));
}

TEST_F(AtomicKMSOutputTest, page_flip_preserves_primary_request_and_uses_normal_commit)
{
    auto output_under_test = output(source_offset);
    auto const properties =
        primary_request_properties({.framebuffer_id = flip_framebuffer_id, .source_offset = source_offset});
    EXPECT_CALL(
        mock_drm, drmModeAtomicCommit(mtd::IsFdOfDevice(drm_device), mtd::AtomicRequestWith(Eq(properties)), no_flags));

    EXPECT_TRUE(output_under_test->page_flip(
        mga::PresentationState::full_scene(std::make_shared<StubFramebuffer>(flip_framebuffer_id))));
}

TEST_P(AtomicKMSFramebufferSizeTest, page_flip_rejects_mismatch_without_allocating_request)
{
    auto output_under_test = output();
    auto const framebuffer = std::make_shared<StubFramebuffer>(flip_framebuffer_id, GetParam().size);
    EXPECT_CALL(mock_drm, drmModeAtomicAlloc()).Times(0);

    EXPECT_FALSE(output_under_test->page_flip(mga::PresentationState::full_scene(framebuffer)));
}

INSTANTIATE_TEST_SUITE_P(
    FramebufferDimensions, AtomicKMSFramebufferSizeTest,
    Values(
        FramebufferMismatch{"WidthMismatch", {smaller_crtc_size.width, requested_mode_size.height}},
        FramebufferMismatch{"HeightMismatch", {requested_mode_size.width, smaller_crtc_size.height}}),
    [](TestParamInfo<FramebufferMismatch> const& info) { return std::string{info.param.name}; });

TEST_F(AtomicKMSOutputTest, power_mode_submits_active_property)
{
    auto output_under_test = output();
    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(ElementsAre(Pair(AtomicPropertyKey{crtc_id, crtc_active_property_id}, 0))),
            DRM_MODE_ATOMIC_ALLOW_MODESET));

    output_under_test->set_power_mode(mir_power_mode_off);
}

TEST_F(AtomicKMSOutputTest, gamma_submits_lut_property)
{
    uint16_t const red_sample{1};
    uint16_t const green_sample{2};
    uint16_t const blue_sample{3};
    auto output_under_test = output();
    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(
                ElementsAre(Pair(AtomicPropertyKey{crtc_id, crtc_gamma_lut_property_id}, gamma_lut_blob_id))),
            DRM_MODE_ATOMIC_ALLOW_MODESET));

    output_under_test->set_gamma({{red_sample}, {green_sample}, {blue_sample}});

    output_under_test.reset();
}

TEST_P(AtomicKMSDisableTest, submits_all_disable_properties)
{
    auto output_under_test = output();
    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(disabled_properties())),
            DRM_MODE_ATOMIC_ALLOW_MODESET));

    if (GetParam().disconnect_connector)
        connector().connection = DRM_MODE_DISCONNECTED;
    ((*output_under_test).*GetParam().operation)();
    EXPECT_THAT(output_under_test->plane_capabilities(), IsNull());

    connector().connection = DRM_MODE_CONNECTED;
    EXPECT_CALL(mock_drm, drmModeGetPropertyBlob(_, plane_formats_blob_id)).Times(1);
    output_under_test->configure({}, 0);
    EXPECT_THAT(output_under_test->plane_capabilities(), NotNull());
}

INSTANTIATE_TEST_SUITE_P(
    DisablePaths, AtomicKMSDisableTest,
    Values(
        DisableCase{"ClearCrtc", &mga::AtomicKMSOutput::clear_crtc},
        DisableCase{"ResetDisconnectedConnector", &mga::AtomicKMSOutput::reset, true},
        DisableCase{"RefreshDisconnectedConnector", &mga::AtomicKMSOutput::refresh_hardware_state, true}),
    [](TestParamInfo<DisableCase> const& info) { return std::string{info.param.name}; });

TEST_F(AtomicKMSOutputTest, set_crtc_commit_failure_returns_false)
{
    auto output_under_test = output();
    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(
                Eq(primary_request_properties({.framebuffer_id = primary_framebuffer_id, .activate_crtc = true}))),
            DRM_MODE_ATOMIC_ALLOW_MODESET))
        .WillOnce(Return(-EIO));

    StubFramebuffer framebuffer{primary_framebuffer_id};
    EXPECT_FALSE(output_under_test->set_crtc(framebuffer));
    EXPECT_THAT(output_under_test->plane_capabilities(), IsNull());
}

TEST_F(AtomicKMSOutputTest, page_flip_commit_failure_returns_false)
{
    auto output_under_test = output();
    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(primary_request_properties({.framebuffer_id = flip_framebuffer_id}))),
            no_flags))
        .WillOnce(Return(-EIO));

    EXPECT_FALSE(output_under_test->page_flip(
        mga::PresentationState::full_scene(std::make_shared<StubFramebuffer>(flip_framebuffer_id))));
}

TEST_F(AtomicKMSOutputTest, initial_capabilities_match_crtc_and_primary_plane)
{
    auto output_under_test = output();
    auto const capabilities = output_under_test->plane_capabilities();
    ASSERT_THAT(capabilities, NotNull());
    EXPECT_THAT(capabilities->crtc_id, Eq(crtc_id));
    EXPECT_THAT(capabilities->primary.id, Eq(plane_id));
}

TEST_F(AtomicKMSOutputTest, failed_flip_clears_capabilities_until_rediscovered)
{
    auto output_under_test = output();
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, no_flags)).WillOnce(Return(-EIO));
    auto const framebuffer = std::make_shared<StubFramebuffer>(flip_framebuffer_id);
    EXPECT_FALSE(output_under_test->page_flip(mga::PresentationState::full_scene(framebuffer)));
    EXPECT_THAT(output_under_test->plane_capabilities(), IsNull());

    EXPECT_CALL(mock_drm, drmModeGetPropertyBlob(_, plane_formats_blob_id)).Times(1);
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, DRM_MODE_ATOMIC_ALLOW_MODESET));
    EXPECT_TRUE(output_under_test->set_crtc(*framebuffer));
    EXPECT_THAT(output_under_test->plane_capabilities(), NotNull());
}

TEST_F(AtomicKMSOutputTest, reset_invalidates_capabilities_even_on_same_connected_crtc)
{
    auto output_under_test = output();
    output_under_test->reset();
    EXPECT_THAT(output_under_test->plane_capabilities(), IsNull());
    EXPECT_CALL(mock_drm, drmModeGetPropertyBlob(_, plane_formats_blob_id)).Times(1);
    output_under_test->configure({}, 0);
    EXPECT_THAT(output_under_test->plane_capabilities(), NotNull());
}

TEST_F(AtomicKMSOutputTest, hardware_refresh_rediscovers_capabilities_on_next_crtc_use)
{
    auto output_under_test = output();
    output_under_test->refresh_hardware_state();
    EXPECT_THAT(output_under_test->plane_capabilities(), IsNull());
    EXPECT_CALL(mock_drm, drmModeGetPropertyBlob(_, plane_formats_blob_id)).Times(1);
    StubFramebuffer framebuffer{primary_framebuffer_id};
    EXPECT_TRUE(output_under_test->set_crtc(framebuffer));
    EXPECT_THAT(output_under_test->plane_capabilities(), NotNull());
}

TEST_F(AtomicKMSOutputTest, failed_discovery_leaves_null_capabilities_but_set_crtc_succeeds)
{
    auto output_under_test = output();
    ASSERT_THAT(output_under_test->plane_capabilities(), NotNull());
    output_under_test->reset();
    StubFramebuffer framebuffer{primary_framebuffer_id};
    EXPECT_CALL(mock_drm, drmModeGetPropertyBlob(_, plane_formats_blob_id))
        .WillOnce(SetErrnoAndReturn(EIO, nullptr));
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, DRM_MODE_ATOMIC_ALLOW_MODESET));
    EXPECT_TRUE(output_under_test->set_crtc(framebuffer));
    EXPECT_THAT(output_under_test->plane_capabilities(), IsNull());
}

TEST_F(AtomicKMSOutputWithoutGammaLutTest, set_gamma_without_gamma_lut_property_is_ignored)
{
    auto output_under_test = output();
    EXPECT_CALL(mock_drm, drmModeCreatePropertyBlob(_, _, _, _)).Times(0);
    EXPECT_CALL(mock_drm, drmModeAtomicAlloc()).Times(0);
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _, _)).Times(0);

    output_under_test->set_gamma(sample_gamma);

    output_under_test.reset();
}

TEST_P(AtomicKMSMixedRequestTest, mixed_presentation_commits_primary_and_overlay_as_test_only)
{
    auto const& param = GetParam();
    add_overlay_plane(param.overlay);
    auto output_under_test = output();
    ASSERT_THAT(output_under_test->plane_capabilities(), NotNull());
    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(mixed_request_properties(param))),
            DRM_MODE_ATOMIC_TEST_ONLY));

    EXPECT_TRUE(output_under_test->test_presentation(mixed_state(param.overlay.id)));
}

// Equal zpos is ordered by plane id, so only an overlay with a lower id than the primary needs primary_zpos + 1.
INSTANTIATE_TEST_SUITE_P(
    MixedRequest,
    AtomicKMSMixedRequestTest,
    Values(
        MixedRequestCase{
            .name = "higher_id_overlay_shares_primary_zpos",
            .overlay = {.id = AtomicKMSOutputTest::plane_id + 1, .zpos_min = 0},
            .expected_overlay_zpos = AtomicKMSOutputTest::primary_zpos},
        MixedRequestCase{
            .name = "lower_id_overlay_goes_above_primary_zpos",
            .overlay = {.id = AtomicKMSOutputTest::plane_id - 1, .zpos_min = 0},
            .expected_overlay_zpos = AtomicKMSOutputTest::primary_zpos + 1},
        MixedRequestCase{
            .name = "overlay_zpos_min_above_primary_is_kept",
            .overlay = {.id = AtomicKMSOutputTest::plane_id + 1, .zpos_min = AtomicKMSOutputTest::primary_zpos + 2},
            .expected_overlay_zpos = AtomicKMSOutputTest::primary_zpos + 2}),
    [](TestParamInfo<MixedRequestCase> const& info) { return std::string{info.param.name}; });

TEST_P(AtomicKMSMixedRejectionTest, test_presentation_rejects_invalid_state)
{
    auto const& param = GetParam();
    add_overlay_plane(
        legal_overlay,
        param.bt709_limited ? mt::EnumValues{mt::color_encodings} : mt::EnumValues{});
    auto output_under_test = output();
    if (param.reset_output)
    {
        output_under_test->reset();
        ASSERT_THAT(output_under_test->plane_capabilities(), IsNull());
    }
    else
    {
        auto const capabilities = output_under_test->plane_capabilities();
        ASSERT_THAT(capabilities, NotNull());
        ASSERT_THAT(capabilities->overlays, SizeIs(1));
        ASSERT_EQ(capabilities->overlays.front().bt709_limited.has_value(), param.bt709_limited);
    }
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);

    EXPECT_FALSE(output_under_test->test_presentation(mixed_state(param.assignment_plane, param.primary_size)));
}

INSTANTIATE_TEST_SUITE_P(
    PresentationValidity,
    AtomicKMSMixedRejectionTest,
    Values(
        MixedPresentationRejectionCase{
            .name = "without_plane_capabilities",
            .primary_size = requested_mode_size,
            .assignment_plane = AtomicKMSOutputTest::plane_id + 1,
            .bt709_limited = true,
            .reset_output = true},
        MixedPresentationRejectionCase{
            .name = "primary_width_mismatch",
            .primary_size = {mode_width - 1, mode_height},
            .assignment_plane = AtomicKMSOutputTest::plane_id + 1,
            .bt709_limited = true,
            .reset_output = false},
        MixedPresentationRejectionCase{
            .name = "primary_height_mismatch",
            .primary_size = {mode_width, mode_height - 1},
            .assignment_plane = AtomicKMSOutputTest::plane_id + 1,
            .bt709_limited = true,
            .reset_output = false},
        MixedPresentationRejectionCase{
            .name = "missing_overlay_plane",
            .primary_size = requested_mode_size,
            .assignment_plane = AtomicKMSOutputTest::plane_id + 2,
            .bt709_limited = true,
            .reset_output = false},
        MixedPresentationRejectionCase{
            .name = "overlay_without_bt709_limited",
            .primary_size = requested_mode_size,
            .assignment_plane = AtomicKMSOutputTest::plane_id + 1,
            .bt709_limited = false,
            .reset_output = false}),
    [](TestParamInfo<MixedPresentationRejectionCase> const& info) { return std::string{info.param.name}; });

TEST_F(AtomicKMSMixedPresentationTest, failed_test_commit_returns_false_and_keeps_capabilities)
{
    add_overlay_plane(legal_overlay);
    auto output_under_test = output();
    ASSERT_THAT(output_under_test->plane_capabilities(), NotNull());
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, DRM_MODE_ATOMIC_TEST_ONLY)).WillOnce(Return(-EINVAL));

    EXPECT_FALSE(output_under_test->test_presentation(mixed_state(legal_overlay.id)));
    EXPECT_THAT(output_under_test->plane_capabilities(), NotNull());
}

TEST_P(AtomicKMSMixedFollowUpTest, follow_up_commit_disables_only_unused_overlays)
{
    add_overlay_plane(legal_overlay);
    auto output_under_test = output();
    ASSERT_THAT(output_under_test->plane_capabilities(), NotNull());

    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(page_flip_keeping_the_overlay(*this))),
            no_flags));
    ASSERT_TRUE(output_under_test->page_flip(mixed_state(legal_overlay.id)));

    Mock::VerifyAndClearExpectations(&mock_drm);

    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(GetParam().expected(*this))),
            GetParam().flags));
    GetParam().follow_up(*this, *output_under_test);
}

INSTANTIATE_TEST_SUITE_P(
    FollowUpActions, AtomicKMSMixedFollowUpTest,
    Values(
        FollowUpCase{
            "FullScenePageFlip", follow_up_with_full_scene_page_flip,
            page_flip_disabling_the_overlay, no_flags},
        FollowUpCase{
            "SetCrtc", follow_up_with_set_crtc,
            set_crtc_disabling_the_overlay, DRM_MODE_ATOMIC_ALLOW_MODESET},
        FollowUpCase{
            "MixedPageFlip", follow_up_with_mixed_page_flip,
            page_flip_keeping_the_overlay, no_flags}),
    [](TestParamInfo<FollowUpCase> const& info) { return std::string{info.param.name}; });

// The failed page flip drops the plane capabilities, so the fallback set_crtc() has to re-ensure
// the CRTC and disable the overlay the previous frame had enabled.
TEST_F(AtomicKMSMixedPresentationTest, failed_full_scene_page_flip_then_set_crtc_disables_the_overlay)
{
    add_overlay_plane(legal_overlay);
    auto output_under_test = output();
    ASSERT_THAT(output_under_test->plane_capabilities(), NotNull());

    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(page_flip_keeping_the_overlay(*this))),
            no_flags));
    ASSERT_TRUE(output_under_test->page_flip(mixed_state(legal_overlay.id)));

    Mock::VerifyAndClearExpectations(&mock_drm);

    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(page_flip_disabling_the_overlay(*this))),
            no_flags))
        .WillOnce(Return(-EIO));
    EXPECT_FALSE(output_under_test->page_flip(mga::PresentationState::full_scene(
        std::make_shared<StubFramebuffer>(flip_framebuffer_id))));

    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(set_crtc_disabling_the_overlay(*this))),
            DRM_MODE_ATOMIC_ALLOW_MODESET));
    EXPECT_TRUE(output_under_test->set_crtc(StubFramebuffer{primary_framebuffer_id}));
}
