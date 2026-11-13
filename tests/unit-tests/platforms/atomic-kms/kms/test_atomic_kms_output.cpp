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
#include "plane_capabilities.h"
#include "presentation_state.h"

#include <mir/graphics/kms/drm_mode_resources.h>
#include <mir/graphics/kms_framebuffer.h>
#include <mir/graphics/gamma_curves.h>
#include <mir/test/doubles/mock_drm.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <drm_fourcc.h>

#include <cerrno>
#include <cstddef>
#include <span>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mga = mir::graphics::atomic;
namespace mg = mir::graphics;
namespace mgk = mir::graphics::kms;
namespace geom = mir::geometry;
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
    static constexpr uint32_t plane_type_property_id{301};
    static constexpr uint32_t plane_src_x_property_id{302};
    static constexpr uint32_t plane_src_y_property_id{303};
    static constexpr uint32_t plane_src_w_property_id{304};
    static constexpr uint32_t plane_src_h_property_id{305};
    static constexpr uint32_t plane_crtc_x_property_id{306};
    static constexpr uint32_t plane_crtc_y_property_id{307};
    static constexpr uint32_t plane_crtc_w_property_id{308};
    static constexpr uint32_t plane_crtc_h_property_id{309};
    static constexpr uint32_t plane_crtc_property_id{310};
    static constexpr uint32_t plane_fb_property_id{311};
    static constexpr uint32_t plane_zpos_property_id{312};
    static constexpr uint32_t plane_formats_property_id{313};
    static constexpr uint32_t plane_formats_blob_id{3};
    static constexpr uint32_t mode_blob_id{1};
    static constexpr uint32_t gamma_lut_blob_id{2};
    static constexpr uint32_t primary_framebuffer_id{77};
    static constexpr uint32_t flip_framebuffer_id{88};
    static constexpr uint32_t connector_id{30};
    static constexpr uint32_t encoder_id{20};
    static constexpr uint32_t crtc_id{10};
    static constexpr uint32_t plane_id{40};
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
            {{plane_id, plane_fb_property_id}, 0},
            {{plane_id, plane_crtc_property_id}, 0}};
    }

    auto primary_request_properties(PrimaryRequestParameters const& parameters) const
        -> std::map<AtomicPropertyKey, uint64_t>
    {
        auto const width = requested_mode_size.width.as_uint32_t();
        auto const height = requested_mode_size.height.as_uint32_t();
        std::map<AtomicPropertyKey, uint64_t> properties{
            {{crtc_id, crtc_mode_property_id}, mode_blob_id}, {{connector_id, connector_crtc_property_id}, crtc_id}};
        if (parameters.activate_crtc)
            properties[{crtc_id, crtc_active_property_id}] = 1;
        properties.insert(
            {{{plane_id, plane_src_x_property_id}, kms_fixed_point(parameters.source_offset.dx.as_uint32_t())},
             {{plane_id, plane_src_y_property_id}, kms_fixed_point(parameters.source_offset.dy.as_uint32_t())},
             {{plane_id, plane_src_w_property_id}, kms_fixed_point(width)},
             {{plane_id, plane_src_h_property_id}, kms_fixed_point(height)},
             {{plane_id, plane_crtc_x_property_id}, 0},
             {{plane_id, plane_crtc_y_property_id}, 0},
             {{plane_id, plane_crtc_w_property_id}, width},
             {{plane_id, plane_crtc_h_property_id}, height},
             {{plane_id, plane_crtc_property_id}, crtc_id},
             {{plane_id, plane_fb_property_id}, parameters.framebuffer_id}});
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
        mock_drm.add_plane(drm_device, plane_id, primary_crtc_mask);
        mock_drm.prepare(drm_device);

        mock_drm.add_property(
            drm_device, connector_id, DRM_MODE_OBJECT_CONNECTOR, connector_crtc_property_id, "CRTC_ID");
        mock_drm.add_property(drm_device, crtc_id, DRM_MODE_OBJECT_CRTC, crtc_active_property_id, "ACTIVE");
        mock_drm.add_property(drm_device, crtc_id, DRM_MODE_OBJECT_CRTC, crtc_mode_property_id, "MODE_ID");
        if (with_gamma_lut)
        {
            mock_drm.add_property(drm_device, crtc_id, DRM_MODE_OBJECT_CRTC, crtc_gamma_lut_property_id, "GAMMA_LUT");
        }
        add_plane_property(plane_type_property_id, "type", DRM_PLANE_TYPE_PRIMARY);
        add_plane_property(plane_src_x_property_id, "SRC_X");
        add_plane_property(plane_src_y_property_id, "SRC_Y");
        add_plane_property(plane_src_w_property_id, "SRC_W");
        add_plane_property(plane_src_h_property_id, "SRC_H");
        add_plane_property(plane_crtc_x_property_id, "CRTC_X");
        add_plane_property(plane_crtc_y_property_id, "CRTC_Y");
        add_plane_property(plane_crtc_w_property_id, "CRTC_W");
        add_plane_property(plane_crtc_h_property_id, "CRTC_H");
        add_plane_property(plane_crtc_property_id, "CRTC_ID");
        add_plane_property(plane_fb_property_id, "FB_ID");
        add_plane_property(plane_zpos_property_id, "zpos");
        mock_drm.set_property_range(
            drm_device, plane_zpos_property_id, DRM_MODE_PROP_RANGE | DRM_MODE_PROP_IMMUTABLE, 0, 0);
        add_plane_property(plane_formats_property_id, "IN_FORMATS", plane_formats_blob_id);
        struct InFormatsBlob
        {
            drm_format_modifier_blob header{
                FORMAT_BLOB_CURRENT, 0, 1, offsetof(InFormatsBlob, formats), 1, offsetof(InFormatsBlob, modifiers)};
            uint32_t formats[1]{DRM_FORMAT_XRGB8888};
            drm_format_modifier modifiers[1]{{1, 0, 0, DRM_FORMAT_MOD_LINEAR}};
        } const formats;
        mock_drm.add_property_blob(drm_device, plane_formats_blob_id, std::as_bytes(std::span{&formats, 1}));
    }

    void add_plane_property(uint32_t id, char const* name, uint64_t value = 0)
    {
        mock_drm.add_property(drm_device, plane_id, DRM_MODE_OBJECT_PLANE, id, name, value);
    }

    std::vector<drmModeModeInfo> modes;
    std::vector<uint32_t> encoder_ids{encoder_id};
};

struct FramebufferMismatch
{
    char const* name;
    geom::Size size;
};

class AtomicKMSFramebufferSizeTest :
    public AtomicKMSOutputTest, public WithParamInterface<FramebufferMismatch>
{
};

struct DisableCase
{
    char const* name;
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

struct MixedRequestCase
{
    char const* name{"baseline"};
    uint32_t overlay_plane{AtomicKMSOutputTest::plane_id + 1};
    uint64_t overlay_zpos_min{0};
    uint64_t expected_overlay_zpos{0};
    geom::Point client_destination{100, 50};
};

/// Adds one overlay plane that can be routed to the fixture's CRTC and can scan out
/// BT.709 limited-range YCbCr, so that mixed presentations are testable
class AtomicKMSMixedPresentationTest : public AtomicKMSOutputTest
{
public:
    static constexpr uint32_t overlay_type_property_id{401};
    static constexpr uint32_t overlay_src_x_property_id{402};
    static constexpr uint32_t overlay_src_y_property_id{403};
    static constexpr uint32_t overlay_src_w_property_id{404};
    static constexpr uint32_t overlay_src_h_property_id{405};
    static constexpr uint32_t overlay_crtc_x_property_id{406};
    static constexpr uint32_t overlay_crtc_y_property_id{407};
    static constexpr uint32_t overlay_crtc_w_property_id{408};
    static constexpr uint32_t overlay_crtc_h_property_id{409};
    static constexpr uint32_t overlay_crtc_property_id{410};
    static constexpr uint32_t overlay_fb_property_id{411};
    static constexpr uint32_t overlay_zpos_property_id{412};
    static constexpr uint32_t overlay_formats_property_id{413};
    static constexpr uint32_t overlay_alpha_property_id{414};
    static constexpr uint32_t overlay_blend_mode_property_id{415};
    static constexpr uint32_t overlay_color_encoding_property_id{416};
    static constexpr uint32_t overlay_color_range_property_id{417};
    static constexpr uint32_t overlay_formats_blob_id{4};

    static constexpr uint64_t overlay_zpos_max{3};
    static constexpr uint64_t overlay_zpos_current{3};
    static constexpr uint64_t overlay_alpha_max{0xffff};
    static constexpr uint64_t overlay_blend_none_value{7};
    static constexpr uint64_t overlay_bt709_encoding_value{11};
    static constexpr uint64_t overlay_bt709_limited_range_value{13};

    static constexpr uint32_t client_framebuffer_id{99};
    static constexpr geom::Size client_framebuffer_size{640, 480};

    MixedRequestCase const request;

    AtomicKMSMixedPresentationTest() : AtomicKMSMixedPresentationTest{MixedRequestCase{}}
    {
    }

    explicit AtomicKMSMixedPresentationTest(MixedRequestCase const& request) : request{request}
    {
        mock_drm.add_plane(drm_device, request.overlay_plane, primary_crtc_mask);
        // prepare() rebuilds the plane list, so re-run it now that the overlay exists
        mock_drm.prepare(drm_device);
        add_overlay_properties();
    }

    auto mixed_state(uint32_t plane, geom::Size primary_size) const -> mga::PresentationState
    {
        return mga::PresentationState::mixed(
            std::make_shared<StubFramebuffer>(primary_framebuffer_id, primary_size),
            {mga::ClientPlaneAssignment{
                plane,
                std::make_shared<StubFramebuffer>(client_framebuffer_id, client_framebuffer_size),
                request.client_destination}});
    }

    auto mixed_state() const -> mga::PresentationState
    {
        return mixed_state(request.overlay_plane, requested_mode_size);
    }

    auto mixed_request_properties() const -> std::map<AtomicPropertyKey, uint64_t>
    {
        auto properties = primary_request_properties({.framebuffer_id = primary_framebuffer_id});
        properties.merge(overlay_request_properties());
        return properties;
    }

private:
    auto overlay_request_properties() const -> std::map<AtomicPropertyKey, uint64_t>
    {
        auto const width = client_framebuffer_size.width.as_uint32_t();
        auto const height = client_framebuffer_size.height.as_uint32_t();
        return {
            {{request.overlay_plane, overlay_fb_property_id}, client_framebuffer_id},
            {{request.overlay_plane, overlay_crtc_property_id}, crtc_id},
            {{request.overlay_plane, overlay_src_x_property_id}, 0},
            {{request.overlay_plane, overlay_src_y_property_id}, 0},
            {{request.overlay_plane, overlay_src_w_property_id}, kms_fixed_point(width)},
            {{request.overlay_plane, overlay_src_h_property_id}, kms_fixed_point(height)},
            {{request.overlay_plane, overlay_crtc_x_property_id},
             static_cast<uint64_t>(static_cast<int64_t>(request.client_destination.x.as_int()))},
            {{request.overlay_plane, overlay_crtc_y_property_id},
             static_cast<uint64_t>(static_cast<int64_t>(request.client_destination.y.as_int()))},
            {{request.overlay_plane, overlay_crtc_w_property_id}, width},
            {{request.overlay_plane, overlay_crtc_h_property_id}, height},
            {{request.overlay_plane, overlay_alpha_property_id}, overlay_alpha_max},
            {{request.overlay_plane, overlay_blend_mode_property_id}, overlay_blend_none_value},
            {{request.overlay_plane, overlay_color_encoding_property_id}, overlay_bt709_encoding_value},
            {{request.overlay_plane, overlay_color_range_property_id}, overlay_bt709_limited_range_value},
            {{request.overlay_plane, overlay_zpos_property_id}, request.expected_overlay_zpos}};
    }

    void add_overlay_property(uint32_t id, char const* name, uint64_t value = 0)
    {
        mock_drm.add_property(drm_device, request.overlay_plane, DRM_MODE_OBJECT_PLANE, id, name, value);
    }

    void add_overlay_properties()
    {
        add_overlay_property(overlay_type_property_id, "type", DRM_PLANE_TYPE_OVERLAY);
        add_overlay_property(overlay_src_x_property_id, "SRC_X");
        add_overlay_property(overlay_src_y_property_id, "SRC_Y");
        add_overlay_property(overlay_src_w_property_id, "SRC_W");
        add_overlay_property(overlay_src_h_property_id, "SRC_H");
        add_overlay_property(overlay_crtc_x_property_id, "CRTC_X");
        add_overlay_property(overlay_crtc_y_property_id, "CRTC_Y");
        add_overlay_property(overlay_crtc_w_property_id, "CRTC_W");
        add_overlay_property(overlay_crtc_h_property_id, "CRTC_H");
        add_overlay_property(overlay_crtc_property_id, "CRTC_ID");
        add_overlay_property(overlay_fb_property_id, "FB_ID");

        add_overlay_property(overlay_zpos_property_id, "zpos", overlay_zpos_current);
        mock_drm.set_property_range(
            drm_device, overlay_zpos_property_id, DRM_MODE_PROP_RANGE, request.overlay_zpos_min, overlay_zpos_max);

        add_overlay_property(overlay_alpha_property_id, "alpha", overlay_alpha_max);
        mock_drm.set_property_range(drm_device, overlay_alpha_property_id, DRM_MODE_PROP_RANGE, 0, overlay_alpha_max);

        std::vector<std::pair<uint64_t, std::string>> const blend_modes{
            {6, "Pre-multiplied"}, {overlay_blend_none_value, "None"}, {8, "Coverage"}};
        add_overlay_property(overlay_blend_mode_property_id, "pixel blend mode");
        mock_drm.set_property_enums(drm_device, overlay_blend_mode_property_id, blend_modes);

        std::vector<std::pair<uint64_t, std::string>> const color_encodings{
            {10, "ITU-R BT.601 YCbCr"},
            {overlay_bt709_encoding_value, "ITU-R BT.709 YCbCr"},
            {12, "ITU-R BT.2020 YCbCr"}};
        add_overlay_property(overlay_color_encoding_property_id, "COLOR_ENCODING");
        mock_drm.set_property_enums(drm_device, overlay_color_encoding_property_id, color_encodings);

        std::vector<std::pair<uint64_t, std::string>> const color_ranges{
            {overlay_bt709_limited_range_value, "YCbCr limited range"}, {14, "YCbCr full range"}};
        add_overlay_property(overlay_color_range_property_id, "COLOR_RANGE");
        mock_drm.set_property_enums(drm_device, overlay_color_range_property_id, color_ranges);

        struct OverlayInFormatsBlob
        {
            drm_format_modifier_blob header{
                FORMAT_BLOB_CURRENT, 0, 1, offsetof(OverlayInFormatsBlob, formats), 1,
                offsetof(OverlayInFormatsBlob, modifiers)};
            uint32_t formats[1]{DRM_FORMAT_NV12};
            drm_format_modifier modifiers[1]{{1, 0, 0, DRM_FORMAT_MOD_LINEAR}};
        } const formats;
        add_overlay_property(overlay_formats_property_id, "IN_FORMATS", overlay_formats_blob_id);
        mock_drm.add_property_blob(drm_device, overlay_formats_blob_id, std::as_bytes(std::span{&formats, 1}));
    }
};

class AtomicKMSMixedRequestTest :
    public AtomicKMSMixedPresentationTest, public WithParamInterface<MixedRequestCase>
{
public:
    AtomicKMSMixedRequestTest() : AtomicKMSMixedPresentationTest{GetParam()}
    {
    }
};

struct MixedPresentationRejectionCase
{
    char const* name;
    geom::Size primary_size{requested_mode_size};
    uint32_t assignment_plane{AtomicKMSOutputTest::plane_id + 1};
    bool bt709_limited{true};
};

class AtomicKMSMixedRejectionTest :
    public AtomicKMSMixedPresentationTest, public WithParamInterface<MixedPresentationRejectionCase>
{
};
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

    StubFramebuffer framebuffer{flip_framebuffer_id};
    EXPECT_TRUE(output_under_test->page_flip(framebuffer));
}

TEST_P(AtomicKMSFramebufferSizeTest, page_flip_rejects_mismatch_without_allocating_request)
{
    auto output_under_test = output();
    StubFramebuffer framebuffer{flip_framebuffer_id, GetParam().size};
    EXPECT_CALL(mock_drm, drmModeAtomicAlloc()).Times(0);

    EXPECT_FALSE(output_under_test->page_flip(framebuffer));
}

INSTANTIATE_TEST_SUITE_P(
    FramebufferDimensions, AtomicKMSFramebufferSizeTest,
    Values(
        FramebufferMismatch{"WidthMismatch", {smaller_crtc_size.width, requested_mode_size.height}},
        FramebufferMismatch{"HeightMismatch", {requested_mode_size.width, smaller_crtc_size.height}}),
    [](TestParamInfo<FramebufferMismatch> const& info) { return info.param.name; });

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
    [](TestParamInfo<DisableCase> const& info) { return info.param.name; });

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

    StubFramebuffer framebuffer{flip_framebuffer_id};
    EXPECT_FALSE(output_under_test->page_flip(framebuffer));
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
    StubFramebuffer framebuffer{flip_framebuffer_id};
    EXPECT_FALSE(output_under_test->page_flip(framebuffer));
    EXPECT_THAT(output_under_test->plane_capabilities(), IsNull());

    EXPECT_CALL(mock_drm, drmModeGetPropertyBlob(_, plane_formats_blob_id)).Times(1);
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, DRM_MODE_ATOMIC_ALLOW_MODESET));
    EXPECT_TRUE(output_under_test->set_crtc(framebuffer));
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
    auto output_under_test = output();
    ASSERT_THAT(output_under_test->plane_capabilities(), NotNull());
    EXPECT_CALL(
        mock_drm,
        drmModeAtomicCommit(
            mtd::IsFdOfDevice(drm_device),
            mtd::AtomicRequestWith(Eq(mixed_request_properties())),
            DRM_MODE_ATOMIC_TEST_ONLY));

    EXPECT_TRUE(output_under_test->test_presentation(mixed_state()));
}

INSTANTIATE_TEST_SUITE_P(
    MixedRequest,
    AtomicKMSMixedRequestTest,
    Values(
        MixedRequestCase{.name = "baseline"},
        MixedRequestCase{.name = "overlay_id_below_primary",
                         .overlay_plane = AtomicKMSOutputTest::plane_id - 1,
                         .expected_overlay_zpos = 1},
        MixedRequestCase{.name = "overlay_min_above_primary",
                         .overlay_zpos_min = 2,
                         .expected_overlay_zpos = 2},
        MixedRequestCase{.name = "negative_destination", .client_destination = {-40, -20}}),
    [](TestParamInfo<MixedRequestCase> const& info) { return info.param.name; });

TEST_P(AtomicKMSMixedRejectionTest, invalid_mixed_presentation_commits_nothing)
{
    auto const& param = GetParam();
    if (!param.bt709_limited)
    {
        mock_drm.set_property_enums(
            drm_device, overlay_color_encoding_property_id,
            {{10, "ITU-R BT.601 YCbCr"}, {12, "ITU-R BT.2020 YCbCr"}});
    }
    auto output_under_test = output();
    auto const capabilities = output_under_test->plane_capabilities();
    ASSERT_THAT(capabilities, NotNull());
    ASSERT_THAT(capabilities->overlays, SizeIs(1));
    ASSERT_EQ(capabilities->overlays.front().bt709_limited.has_value(), param.bt709_limited);

    auto const state = mixed_state(param.assignment_plane, param.primary_size);
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);

    EXPECT_FALSE(output_under_test->test_presentation(state));
}

INSTANTIATE_TEST_SUITE_P(
    PresentationValidity,
    AtomicKMSMixedRejectionTest,
    Values(
        MixedPresentationRejectionCase{.name = "primary_width_mismatch",
                                       .primary_size = {mode_width - 1, mode_height}},
        MixedPresentationRejectionCase{.name = "primary_height_mismatch",
                                       .primary_size = {mode_width, mode_height - 1}},
        MixedPresentationRejectionCase{.name = "missing_overlay_plane",
                                       .assignment_plane = AtomicKMSOutputTest::plane_id + 2},
        MixedPresentationRejectionCase{.name = "overlay_without_bt709_limited", .bt709_limited = false}),
    [](TestParamInfo<MixedPresentationRejectionCase> const& info) { return info.param.name; });

TEST_F(AtomicKMSMixedPresentationTest, failed_test_commit_returns_false_and_keeps_capabilities)
{
    auto output_under_test = output();
    ASSERT_THAT(output_under_test->plane_capabilities(), NotNull());
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, DRM_MODE_ATOMIC_TEST_ONLY)).WillOnce(Return(-EINVAL));

    EXPECT_FALSE(output_under_test->test_presentation(mixed_state()));
    EXPECT_THAT(output_under_test->plane_capabilities(), NotNull());
}

TEST_F(AtomicKMSMixedPresentationTest, test_presentation_without_plane_capabilities_commits_nothing)
{
    auto output_under_test = output();
    output_under_test->reset();
    ASSERT_THAT(output_under_test->plane_capabilities(), IsNull());
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);

    EXPECT_FALSE(output_under_test->test_presentation(mixed_state()));
}
