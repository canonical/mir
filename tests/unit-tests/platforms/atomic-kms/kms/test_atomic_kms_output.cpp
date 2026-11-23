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
#include "cursor.h"
#include "fake_plane_properties.h"
#include "kms_output_container.h"
#include "plane_capabilities.h"
#include "presentation_state.h"

#include <mir/geometry/rectangle.h>
#include <mir/graphics/kms/drm_mode_resources.h>
#include <mir/graphics/kms_framebuffer.h>
#include <mir/graphics/gamma_curves.h>
#include <mir/test/doubles/mock_drm.h>
#include <mir/test/doubles/mock_gbm.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <drm_fourcc.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <future>
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
    static constexpr uint32_t cursor_plane_id{41};
    static constexpr uint32_t bound_cursor_plane_id{42};
    // FakePlaneProperties uses the plane ID as its IN_FORMATS blob ID
    static constexpr uint32_t plane_formats_blob_id{plane_id};
    static constexpr mt::PlanePropertyIds primary_properties{plane_id};
    static constexpr uint64_t primary_zpos{0};
    static constexpr mt::PlanePropertyIds cursor_ids{cursor_plane_id};
    static constexpr mt::PlanePropertyIds bound_cursor_ids{bound_cursor_plane_id};
    static constexpr uint32_t primary_crtc_mask{1};

    AtomicKMSOutputTest()
        : AtomicKMSOutputTest{true}
    {
    }

    explicit AtomicKMSOutputTest(bool with_gamma_lut, bool with_cursor = false, bool with_bound_cursor = false)
        : drm_fd{mock_drm.open(drm_device, 0)}, with_cursor{with_cursor}, with_bound_cursor{with_bound_cursor}
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
        if (with_cursor)
            plane_properties.add(
                {.id = cursor_plane_id, .crtcs = primary_crtc_mask, .type = DRM_PLANE_TYPE_CURSOR, .in_formats = false},
                {});
        if (with_bound_cursor)
            plane_properties.add(
                {.id = bound_cursor_plane_id, .crtcs = primary_crtc_mask, .type = DRM_PLANE_TYPE_CURSOR,
                 .in_formats = false, .bound_crtc = crtc_id},
                {});
        mock_drm.prepare(drm_device);
        if (with_cursor)
        {
            auto const cursor_plane = drmModeGetPlane(drm_fd, cursor_plane_id);
            cursor_plane->formats = &cursor_format;
            cursor_plane->count_formats = 1;
        }
        if (with_bound_cursor)
        {
            auto const cursor_plane = drmModeGetPlane(drm_fd, bound_cursor_plane_id);
            cursor_plane->formats = &cursor_format;
            cursor_plane->count_formats = 1;
            cursor_plane->crtc_id = crtc_id;
        }

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
    uint32_t cursor_format{DRM_FORMAT_ARGB8888};
    bool const with_cursor;
    bool const with_bound_cursor;
};

class AtomicKMSCursorTest : public AtomicKMSOutputTest
{
public:
    static constexpr uint32_t cursor_fb_id{99};
    static constexpr uint32_t cursor_width{64};
    static constexpr uint32_t cursor_height{64};

    explicit AtomicKMSCursorTest(bool with_bound_cursor = false) : AtomicKMSOutputTest{true, true, with_bound_cursor}
    {
        ON_CALL(gbm, gbm_bo_get_width(_)).WillByDefault(Return(cursor_width));
        ON_CALL(gbm, gbm_bo_get_height(_)).WillByDefault(Return(cursor_height));
        ON_CALL(gbm, gbm_bo_get_stride(_)).WillByDefault(Return(cursor_width * 4));
        ON_CALL(gbm, gbm_bo_get_format(_)).WillByDefault(Return(DRM_FORMAT_ARGB8888));
        ON_CALL(mock_drm, drmModeAddFB2(_, _, _, _, _, _, _, _, _))
            .WillByDefault(DoAll(SetArgPointee<7>(cursor_fb_id), Return(0)));
        EXPECT_CALL(mock_drm, drmModeSetCursor(_, _, _, _, _)).Times(0);
        EXPECT_CALL(mock_drm, drmModeMoveCursor(_, _, _, _)).Times(0);
    }

    auto cursor_properties(geom::Point position = {}) const -> std::map<AtomicPropertyKey, uint64_t>
    {
        return {
            {{cursor_plane_id, cursor_ids.fb_id}, cursor_fb_id},
            {{cursor_plane_id, cursor_ids.crtc_id}, crtc_id},
            {{cursor_plane_id, cursor_ids.src_x}, 0},
            {{cursor_plane_id, cursor_ids.src_y}, 0},
            {{cursor_plane_id, cursor_ids.src_w}, kms_fixed_point(cursor_width)},
            {{cursor_plane_id, cursor_ids.src_h}, kms_fixed_point(cursor_height)},
            {{cursor_plane_id, cursor_ids.crtc_x}, static_cast<uint64_t>(position.x.as_int())},
            {{cursor_plane_id, cursor_ids.crtc_y}, static_cast<uint64_t>(position.y.as_int())},
            {{cursor_plane_id, cursor_ids.crtc_w}, cursor_width},
            {{cursor_plane_id, cursor_ids.crtc_h}, cursor_height}};
    }

    NiceMock<mtd::MockGBM> gbm;
};

class AtomicKMSBoundCursorTest : public AtomicKMSCursorTest
{
public:
    AtomicKMSBoundCursorTest() : AtomicKMSCursorTest{true} {}
};

enum class CursorCommit
{
    cursor_only,
    page_flip,
    modeset
};

class AtomicKMSConcurrentCursorTest :
    public AtomicKMSCursorTest, public WithParamInterface<CursorCommit>
{
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
}

TEST_F(AtomicKMSCursorTest, image_and_moves_are_deferred_and_coalesced)
{
    auto output_under_test = output();
    int refreshes{0};
    output_under_test->register_refresh_handler([&]
        {
            ++refreshes;
            // The refresh callback may inspect the output without deadlocking.
            EXPECT_TRUE(output_under_test->has_cursor_image());
        });
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    output_under_test->move_cursor({1, 2});
    output_under_test->move_cursor({-3, 4});
    EXPECT_TRUE(output_under_test->has_cursor_image());
    EXPECT_THAT(refreshes, Eq(1));
    Mock::VerifyAndClearExpectations(&mock_drm);

    EXPECT_CALL(mock_drm, drmModeAtomicCommit(
        _, mtd::AtomicRequestWith(Eq(cursor_properties({-3, 4}))), no_flags));
    EXPECT_TRUE(output_under_test->post_cursor());
    Mock::VerifyAndClearExpectations(&mock_drm);
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSCursorTest, cursor_and_content_share_page_flip)
{
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    auto properties = primary_request_properties({.framebuffer_id = flip_framebuffer_id});
    properties.merge(cursor_properties());
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, mtd::AtomicRequestWith(Eq(properties)), no_flags));
    StubFramebuffer framebuffer{flip_framebuffer_id};
    EXPECT_TRUE(output_under_test->page_flip(framebuffer));
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSCursorTest, cursor_and_content_share_modeset)
{
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    auto properties = primary_request_properties(
        {.framebuffer_id = primary_framebuffer_id, .activate_crtc = true});
    properties.merge(cursor_properties());
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(
        _, mtd::AtomicRequestWith(Eq(properties)), DRM_MODE_ATOMIC_ALLOW_MODESET));
    StubFramebuffer framebuffer{primary_framebuffer_id};
    EXPECT_TRUE(output_under_test->set_crtc(framebuffer));
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSCursorTest, clean_cursor_does_not_add_properties_to_content_commit)
{
    auto output_under_test = output();
    auto const properties = primary_request_properties({.framebuffer_id = flip_framebuffer_id});
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, mtd::AtomicRequestWith(Eq(properties)), no_flags));
    StubFramebuffer framebuffer{flip_framebuffer_id};
    EXPECT_TRUE(output_under_test->page_flip(framebuffer));
}

TEST_F(AtomicKMSCursorTest, failed_cursor_commit_is_retried_and_new_move_requests_refresh)
{
    auto output_under_test = output();
    int refreshes{0};
    output_under_test->register_refresh_handler([&] { ++refreshes; });
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(
        _, mtd::AtomicRequestWith(Eq(cursor_properties())), no_flags)).WillOnce(Return(-EACCES));
    EXPECT_FALSE(output_under_test->post_cursor());
    output_under_test->move_cursor({3, 4});
    EXPECT_THAT(refreshes, Eq(2));
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(
        _, mtd::AtomicRequestWith(Eq(cursor_properties({3, 4}))), no_flags));
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSCursorTest, failed_content_commit_keeps_pending_cursor_for_modeset)
{
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, no_flags)).WillOnce(Return(-EINVAL));
    StubFramebuffer framebuffer{flip_framebuffer_id};
    EXPECT_FALSE(output_under_test->page_flip(framebuffer));
    auto properties = primary_request_properties(
        {.framebuffer_id = flip_framebuffer_id, .activate_crtc = true});
    properties.merge(cursor_properties());
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(
        _, mtd::AtomicRequestWith(Eq(properties)), DRM_MODE_ATOMIC_ALLOW_MODESET));
    EXPECT_TRUE(output_under_test->set_crtc(framebuffer));
}

TEST_F(AtomicKMSCursorTest, clearing_retains_visible_framebuffer_until_successful_commit)
{
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    ASSERT_TRUE(output_under_test->post_cursor());
    EXPECT_CALL(mock_drm, drmModeRmFB(_, cursor_fb_id)).Times(0);
    EXPECT_TRUE(output_under_test->clear_cursor());
    EXPECT_FALSE(output_under_test->has_cursor_image());
    std::map<AtomicPropertyKey, uint64_t> const properties{
        {{cursor_plane_id, cursor_ids.fb_id}, 0},
        {{cursor_plane_id, cursor_ids.crtc_id}, 0}};
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(
        _, mtd::AtomicRequestWith(Eq(properties)), no_flags)).WillOnce(Return(-EACCES));
    EXPECT_FALSE(output_under_test->post_cursor());
    Mock::VerifyAndClearExpectations(&mock_drm);
    InSequence sequence;
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, mtd::AtomicRequestWith(Eq(properties)), no_flags));
    EXPECT_CALL(mock_drm, drmModeRmFB(_, cursor_fb_id));
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSCursorTest, registration_wakes_for_already_pending_cursor)
{
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    int refreshes{0};
    output_under_test->register_refresh_handler([&] { ++refreshes; });
    EXPECT_THAT(refreshes, Eq(1));
}

TEST_F(AtomicKMSCursorTest, clearing_refresh_handler_waits_for_in_flight_callback)
{
    using namespace std::chrono_literals;
    auto output_under_test = output();
    std::promise<void> entered;
    std::promise<void> release;
    auto released = release.get_future().share();
    std::atomic<int> calls{0};
    output_under_test->register_refresh_handler([&]
        {
            if (++calls == 1)
                entered.set_value();
            released.wait();
        });
    auto update = std::async(std::launch::async, [&]
        {
            output_under_test->set_cursor_image(gbm.fake_gbm.bo);
        });
    EXPECT_THAT(entered.get_future().wait_for(2s), Eq(std::future_status::ready));
    std::promise<void> clearing;
    auto clear = std::async(std::launch::async, [&]
        {
            clearing.set_value();
            output_under_test->register_refresh_handler({});
        });
    clearing.get_future().wait();
    EXPECT_THAT(clear.wait_for(50ms), Eq(std::future_status::timeout));
    release.set_value();
    update.get();
    clear.get();
    EXPECT_TRUE(output_under_test->post_cursor());
    output_under_test->move_cursor({1, 2});
    EXPECT_THAT(calls.load(), Eq(1));
}

TEST_P(AtomicKMSConcurrentCursorTest, input_updates_do_not_wait_for_commit_and_are_not_lost)
{
    using namespace std::chrono_literals;
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    std::atomic<int> refreshes{0};
    output_under_test->register_refresh_handler([&] { ++refreshes; });
    std::promise<void> entered;
    std::promise<void> release;
    auto released = release.get_future().share();
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).WillOnce(
        [&] (auto...)
        {
            entered.set_value();
            released.wait();
            return 0;
        });
    EXPECT_CALL(mock_drm, drmModeRmFB(_, cursor_fb_id)).Times(0);
    StubFramebuffer framebuffer{flip_framebuffer_id};
    auto commit = std::async(std::launch::async, [&]
        {
            switch (GetParam())
            {
            case CursorCommit::cursor_only:
                return output_under_test->post_cursor();
            case CursorCommit::page_flip:
                return output_under_test->page_flip(framebuffer);
            case CursorCommit::modeset:
                return output_under_test->set_crtc(framebuffer);
            }
            return false;
        });
    EXPECT_THAT(entered.get_future().wait_for(2s), Eq(std::future_status::ready));
    ON_CALL(mock_drm, drmModeAddFB2(_, _, _, _, _, _, _, _, _))
        .WillByDefault(DoAll(SetArgPointee<7>(100u), Return(0)));
    auto input = std::async(std::launch::async, [&]
        {
            EXPECT_THAT(output_under_test->id(), Eq(connector_id));
            EXPECT_THAT(output_under_test->size(), Eq(requested_mode_size));
            EXPECT_TRUE(output_under_test->has_cursor_image());
            output_under_test->move_cursor({1, 2});
            EXPECT_TRUE(output_under_test->clear_cursor());
            output_under_test->set_cursor_image(gbm.fake_gbm.bo);
            output_under_test->move_cursor({-3, 4});
        });
    EXPECT_THAT(input.wait_for(2s), Eq(std::future_status::ready));
    release.set_value();
    input.get();
    EXPECT_TRUE(commit.get());
    EXPECT_THAT(refreshes.load(), Eq(2));
    Mock::VerifyAndClearExpectations(&mock_drm);

    auto properties = cursor_properties({-3, 4});
    properties[{cursor_plane_id, cursor_ids.fb_id}] = 100;
    InSequence sequence;
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, mtd::AtomicRequestWith(Eq(properties)), no_flags));
    EXPECT_CALL(mock_drm, drmModeRmFB(_, cursor_fb_id));
    EXPECT_TRUE(output_under_test->post_cursor());
    EXPECT_CALL(mock_drm, drmModeRmFB(_, 100u));
}

INSTANTIATE_TEST_SUITE_P(
    Commits,
    AtomicKMSConcurrentCursorTest,
    Values(CursorCommit::cursor_only, CursorCommit::page_flip, CursorCommit::modeset));

TEST_F(AtomicKMSCursorTest, replacing_pending_image_keeps_visible_image_until_commit)
{
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    ASSERT_TRUE(output_under_test->post_cursor());
    EXPECT_CALL(mock_drm, drmModeRmFB(_, cursor_fb_id)).Times(0);
    EXPECT_CALL(mock_drm, drmModeAddFB2(_, _, _, _, _, _, _, _, _))
        .WillOnce(DoAll(SetArgPointee<7>(100u), Return(0)))
        .WillOnce(DoAll(SetArgPointee<7>(101u), Return(0)));
    EXPECT_CALL(mock_drm, drmModeRmFB(_, 100u));
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    Mock::VerifyAndClearExpectations(&mock_drm);
    InSequence sequence;
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, no_flags));
    EXPECT_CALL(mock_drm, drmModeRmFB(_, cursor_fb_id));
    EXPECT_TRUE(output_under_test->post_cursor());
    EXPECT_CALL(mock_drm, drmModeRmFB(_, 101u));
}

TEST_F(AtomicKMSCursorTest, posting_does_not_access_the_original_gbm_buffer)
{
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    gbm_bo_destroy(gbm.fake_gbm.bo);
    EXPECT_CALL(gbm, gbm_bo_get_width(_)).Times(0);
    EXPECT_CALL(gbm, gbm_bo_get_height(_)).Times(0);
    EXPECT_CALL(gbm, gbm_bo_get_handle(_)).Times(0);
    EXPECT_TRUE(output_under_test->post_cursor());
    EXPECT_TRUE(output_under_test->clear_cursor());
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSCursorTest, reconfiguration_reapplies_the_cursor_with_the_content)
{
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    ASSERT_TRUE(output_under_test->post_cursor());
    output_under_test->reset();
    output_under_test->configure({}, 0);
    auto properties = primary_request_properties(
        {.framebuffer_id = primary_framebuffer_id, .activate_crtc = true});
    properties[{crtc_id, crtc_mode_property_id}] = mode_blob_id + 1;
    properties.merge(cursor_properties());
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(
        _, mtd::AtomicRequestWith(Eq(properties)), DRM_MODE_ATOMIC_ALLOW_MODESET));
    StubFramebuffer framebuffer{primary_framebuffer_id};
    EXPECT_TRUE(output_under_test->set_crtc(framebuffer));
}

TEST_F(AtomicKMSCursorTest, incompatible_cursor_plane_is_not_used)
{
    auto const plane = drmModeGetPlane(drm_fd, cursor_plane_id);
    plane->possible_crtcs = primary_crtc_mask << 1;
    auto output_under_test = output();
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    EXPECT_FALSE(output_under_test->has_cursor_image());
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSCursorTest, unbound_cursor_plane_shared_by_multiple_crtcs_is_not_claimed)
{
    auto const plane = drmModeGetPlane(drm_fd, cursor_plane_id);
    plane->possible_crtcs = primary_crtc_mask | (primary_crtc_mask << 1);
    auto output_under_test = output();
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);
    EXPECT_CALL(mock_drm, drmModeAddFB2(_, _, _, _, _, _, _, _, _)).Times(0);
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    EXPECT_FALSE(output_under_test->has_cursor_image());
    EXPECT_FALSE(output_under_test->clear_cursor());
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSBoundCursorTest, bound_cursor_plane_is_preferred_over_earlier_unbound_plane)
{
    auto const plane = drmModeGetPlane(drm_fd, bound_cursor_plane_id);
    plane->possible_crtcs = primary_crtc_mask | (primary_crtc_mask << 1);
    auto output_under_test = output();
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    std::map<AtomicPropertyKey, uint64_t> properties;
    for (auto const& [key, value] : cursor_properties())
        properties[{bound_cursor_plane_id, key.property_id - cursor_ids.type + bound_cursor_ids.type}] = value;
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, mtd::AtomicRequestWith(Eq(properties)), no_flags));
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSCursorTest, constructor_hide_defers_clearing_an_inherited_cursor)
{
    struct Outputs : mga::KMSOutputContainer
    {
        std::shared_ptr<mga::KMSOutput> output;
        void for_each_output(std::function<void(std::shared_ptr<mga::KMSOutput> const&)> f) const override
        {
            f(output);
        }
        void update_from_hardware_state() override {}
    };
    struct InitialConfiguration : mga::CurrentConfiguration
    {
        void with_current_configuration_do(
            std::function<void(mga::KMSDisplayConfiguration const&)> const&) override {}
    };
    auto const props = drmModeObjectGetProperties(drm_fd, cursor_plane_id, DRM_MODE_OBJECT_PLANE);
    for (uint32_t i = 0; i < props->count_props; ++i)
    {
        if (props->props[i] == cursor_ids.fb_id)
            props->prop_values[i] = cursor_fb_id;
        if (props->props[i] == cursor_ids.crtc_id)
            props->prop_values[i] = crtc_id;
    }
    auto outputs = std::make_shared<Outputs>();
    outputs->output = output();
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);
    mga::Cursor cursor{outputs, std::make_shared<InitialConfiguration>()};
    Mock::VerifyAndClearExpectations(&mock_drm);
    std::map<AtomicPropertyKey, uint64_t> const properties{
        {{cursor_plane_id, cursor_ids.fb_id}, 0},
        {{cursor_plane_id, cursor_ids.crtc_id}, 0}};
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, mtd::AtomicRequestWith(Eq(properties)), no_flags));
    EXPECT_TRUE(outputs->output->post_cursor());
}

TEST_F(AtomicKMSCursorTest, cursor_plane_without_argb_support_is_not_used)
{
    auto const plane = drmModeGetPlane(drm_fd, cursor_plane_id);
    plane->count_formats = 0;
    auto output_under_test = output();
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    EXPECT_FALSE(output_under_test->has_cursor_image());
    EXPECT_TRUE(output_under_test->post_cursor());
}

TEST_F(AtomicKMSOutputTest, missing_cursor_plane_does_not_issue_cursor_commit)
{
    auto output_under_test = output();
    NiceMock<mtd::MockGBM> gbm;
    EXPECT_CALL(mock_drm, drmModeAtomicCommit(_, _, _)).Times(0);
    EXPECT_CALL(mock_drm, drmModeAddFB2(_, _, _, _, _, _, _, _, _)).Times(0);
    output_under_test->set_cursor_image(gbm.fake_gbm.bo);
    output_under_test->move_cursor({10, 20});
    EXPECT_FALSE(output_under_test->has_cursor_image());
    EXPECT_FALSE(output_under_test->clear_cursor());
    EXPECT_TRUE(output_under_test->post_cursor());
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

TEST_F(AtomicKMSOutputTest, hardware_refresh_rediscovers_capabilities_immediately)
{
    auto output_under_test = output();
    EXPECT_CALL(mock_drm, drmModeGetPropertyBlob(_, plane_formats_blob_id)).Times(1);
    output_under_test->refresh_hardware_state();
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
