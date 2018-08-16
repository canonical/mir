/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/platforms/mesa/server/kms/real_kms_output.h"
#include "src/platforms/mesa/server/kms/page_flipper.h"
#include "mir/fatal.h"

#include "mir/test/fake_shared.h"

#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"

#include <stdexcept>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fcntl.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{

class NullPageFlipper : public mgm::PageFlipper
{
public:
    bool schedule_flip(uint32_t,uint32_t,uint32_t) override { return true; }
    mg::Frame wait_for_flip(uint32_t) override { return {}; }
};

class MockPageFlipper : public mgm::PageFlipper
{
public:
    MOCK_METHOD3(schedule_flip, bool(uint32_t,uint32_t,uint32_t));
    MOCK_METHOD1(wait_for_flip, mg::Frame(uint32_t));
};

class RealKMSOutputTest : public ::testing::Test
{
public:
    RealKMSOutputTest()
        : drm_fd{open(drm_device, 0, 0)},
          invalid_id{0}, crtc_ids{10, 11},
          encoder_ids{20, 21}, connector_ids{30, 21},
          possible_encoder_ids1{encoder_ids[0]},
          possible_encoder_ids2{encoder_ids[0], encoder_ids[1]}
    {
        ON_CALL(mock_page_flipper, wait_for_flip(_))
            .WillByDefault(Return(mg::Frame{}));

        ON_CALL(mock_gbm, gbm_bo_get_handle(_))
            .WillByDefault(Return(gbm_bo_handle{0}));
    }

    void setup_outputs_connected_crtc()
    {
        uint32_t const possible_crtcs_mask{0x1};

        mock_drm.reset(drm_device);

        mock_drm.add_crtc(
            drm_device,
            crtc_ids[0],
            drmModeModeInfo());
        mock_drm.add_encoder(
            drm_device,
            encoder_ids[0],
            crtc_ids[0],
            possible_crtcs_mask);
        mock_drm.add_connector(
            drm_device,
            connector_ids[0],
            DRM_MODE_CONNECTOR_VGA,
            DRM_MODE_CONNECTED,
            encoder_ids[0],
            modes_empty,
            possible_encoder_ids1,
            geom::Size());

        mock_drm.prepare(drm_device);
    }

    void setup_outputs_no_connected_crtc()
    {
        uint32_t const possible_crtcs_mask1{0x1};
        uint32_t const possible_crtcs_mask_all{0x3};

        mock_drm.reset(drm_device);

        mock_drm.add_crtc(
            drm_device,
            crtc_ids[0],
            drmModeModeInfo());
        mock_drm.add_crtc(
            drm_device,
            crtc_ids[1],
            drmModeModeInfo());
        mock_drm.add_encoder(
            drm_device,
            encoder_ids[0],
            crtc_ids[0],
            possible_crtcs_mask1);
        mock_drm.add_encoder(
            drm_device,
            encoder_ids[1],
            invalid_id,
            possible_crtcs_mask_all);
        mock_drm.add_connector(
            drm_device,
            connector_ids[0],
            DRM_MODE_CONNECTOR_Composite,
            DRM_MODE_CONNECTED,
            invalid_id,
            modes_empty,
            possible_encoder_ids2,
            geom::Size());
        mock_drm.add_connector(
            drm_device,
            connector_ids[1],
            DRM_MODE_CONNECTOR_DVIA,
            DRM_MODE_CONNECTED,
            encoder_ids[0],
            modes_empty,
            possible_encoder_ids2,
            geom::Size());

        mock_drm.prepare(drm_device);
    }

    void append_fb_id(uint32_t fb_id)
    {
        EXPECT_CALL(mock_drm, drmModeAddFB2(_,_,_,_,_,_,_,_,_))
            .WillOnce(
                DoAll(
                    SetArgPointee<7>(fb_id),
                    Return(0)));
    }

    testing::NiceMock<mtd::MockDRM> mock_drm;
    testing::NiceMock<mtd::MockGBM> mock_gbm;
    MockPageFlipper mock_page_flipper;
    NullPageFlipper null_page_flipper;
    std::vector<drmModeModeInfo> modes_empty;

    char const* const drm_device = "/dev/dri/card0";
    int const drm_fd;

    gbm_bo* const fake_bo{reinterpret_cast<gbm_bo*>(0x123ba)};
    uint32_t const invalid_id;
    std::vector<uint32_t> const crtc_ids;
    std::vector<uint32_t> const encoder_ids;
    std::vector<uint32_t> const connector_ids;
    std::vector<uint32_t> possible_encoder_ids1;
    std::vector<uint32_t> possible_encoder_ids2;
};

}

TEST_F(RealKMSOutputTest, operations_use_existing_crtc)
{
    using namespace testing;

    setup_outputs_connected_crtc();

    uint32_t const fb_id{42};
    append_fb_id(fb_id);

    {
        InSequence s;

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], fb_id, _, _,
                                             Pointee(connector_ids[0]), _, _))
            .Times(1);

        EXPECT_CALL(mock_page_flipper, schedule_flip(crtc_ids[0], fb_id,
                                                     connector_ids[0]))
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_CALL(mock_page_flipper, wait_for_flip(crtc_ids[0]))
            .Times(1);

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], Ne(fb_id), _, _,
                                             Pointee(connector_ids[0]), _, _))
            .Times(1);
    }

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    auto fb = output.fb_for(fake_bo);

    EXPECT_TRUE(output.set_crtc(*fb));
    EXPECT_TRUE(output.schedule_page_flip(*fb));
    output.wait_for_page_flip();
}

TEST_F(RealKMSOutputTest, operations_use_possible_crtc)
{
    using namespace testing;

    uint32_t const fb_id{67};

    setup_outputs_no_connected_crtc();

    {
        InSequence s;

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[1], fb_id, _, _,
                                             Pointee(connector_ids[0]), _, _))
            .Times(1);

        EXPECT_CALL(mock_page_flipper, schedule_flip(crtc_ids[1], fb_id,
                                                     connector_ids[0]))
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_CALL(mock_page_flipper, wait_for_flip(crtc_ids[1]))
            .Times(1);

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, 0, 0, _, _,
                                             Pointee(connector_ids[0]), _, _))
            .Times(1);
    }

    append_fb_id(fb_id);

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    auto fb = output.fb_for(fake_bo);

    EXPECT_TRUE(output.set_crtc(*fb));
    EXPECT_TRUE(output.schedule_page_flip(*fb));
    output.wait_for_page_flip();
}

TEST_F(RealKMSOutputTest, set_crtc_failure_is_handled_gracefully)
{
    mir::FatalErrorStrategy on_error{mir::fatal_error_except};
    using namespace testing;

    uint32_t const fb_id{67};

    setup_outputs_connected_crtc();

    {
        InSequence s;

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(1));

        EXPECT_CALL(mock_page_flipper, schedule_flip(_, _, _))
            .Times(0);

        EXPECT_CALL(mock_page_flipper, wait_for_flip(_))
            .Times(0);

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, _, _, _, _, _, _, _))
            .Times(0);
    }

    append_fb_id(fb_id);

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    auto fb = output.fb_for(fake_bo);

    EXPECT_FALSE(output.set_crtc(*fb));

    EXPECT_NO_THROW({
        EXPECT_FALSE(output.schedule_page_flip(*fb));
    });
    EXPECT_THROW({  // schedule failed. It's programmer error if you then wait.
        output.wait_for_page_flip();
    }, std::runtime_error);
}

TEST_F(RealKMSOutputTest, clear_crtc_gets_crtc_if_none_is_current)
{
    using namespace testing;

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], 0, 0, 0, nullptr, 0, nullptr))
        .Times(1)
        .WillOnce(Return(0));

    output.clear_crtc();
}

TEST_F(RealKMSOutputTest, clear_crtc_does_not_throw_if_no_crtc_is_found)
{
    using namespace testing;

    uint32_t const possible_crtcs_mask_empty{0x0};

    mock_drm.reset(drm_device);

    mock_drm.add_encoder(
        drm_device,
        encoder_ids[0],
        invalid_id,
        possible_crtcs_mask_empty);
    mock_drm.add_connector(
        drm_device,
        connector_ids[0],
        DRM_MODE_CONNECTOR_VGA,
        DRM_MODE_CONNECTED,
        encoder_ids[0],
        modes_empty,
        possible_encoder_ids1,
        geom::Size());

    mock_drm.prepare(drm_device);

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    EXPECT_CALL(mock_drm, drmModeSetCrtc(_, _, 0, 0, 0, nullptr, 0, nullptr))
        .Times(0);

    output.clear_crtc();
}

TEST_F(RealKMSOutputTest, cursor_move_permission_failure_is_non_fatal)
{   // Regression test for LP: #1579630
    using namespace testing;

    EXPECT_CALL(mock_drm, drmModeMoveCursor(_, _, _, _))
        .Times(1)
        .WillOnce(Return(-13));

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    auto fb = output.fb_for(fake_bo);

    EXPECT_TRUE(output.set_crtc(*fb));
    EXPECT_NO_THROW({
        output.move_cursor({123, 456});
    });
}

TEST_F(RealKMSOutputTest, cursor_set_permission_failure_is_non_fatal)
{   // Regression test for LP: #1579630
    using namespace testing;

    EXPECT_CALL(mock_drm, drmModeSetCursor(_, _, _, _, _))
        .Times(1)
        .WillOnce(Return(-13));
    union gbm_bo_handle some_handle;
    some_handle.u64 = 0xbaadf00d;
    ON_CALL(mock_gbm, gbm_bo_get_handle(_)).WillByDefault(Return(some_handle));
    ON_CALL(mock_gbm, gbm_bo_get_width(_)).WillByDefault(Return(34));
    ON_CALL(mock_gbm, gbm_bo_get_height(_)).WillByDefault(Return(56));

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    auto fb = output.fb_for(fake_bo);

    EXPECT_TRUE(output.set_crtc(*fb));
    struct gbm_bo *dummy = reinterpret_cast<struct gbm_bo*>(0x1234567);
    EXPECT_NO_THROW({
        output.set_cursor(dummy);
    });
}

TEST_F(RealKMSOutputTest, has_no_cursor_if_no_hardware_support)
{   // Regression test related to LP: #1610054
    using namespace testing;

    EXPECT_CALL(mock_drm, drmModeSetCursor(_, _, _, _, _))
        .Times(1)
        .WillOnce(Return(-ENXIO));
    union gbm_bo_handle some_handle;
    some_handle.u64 = 0xbaadf00d;
    ON_CALL(mock_gbm, gbm_bo_get_handle(_)).WillByDefault(Return(some_handle));
    ON_CALL(mock_gbm, gbm_bo_get_width(_)).WillByDefault(Return(34));
    ON_CALL(mock_gbm, gbm_bo_get_height(_)).WillByDefault(Return(56));

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    auto fb = output.fb_for(fake_bo);

    EXPECT_TRUE(output.set_crtc(*fb));
    struct gbm_bo *dummy = reinterpret_cast<struct gbm_bo*>(0x1234567);
    output.set_cursor(dummy);
    EXPECT_FALSE(output.has_cursor());
}

TEST_F(RealKMSOutputTest, clear_crtc_is_non_fatal_on_permission_error)
{
    mir::FatalErrorStrategy on_error{mir::fatal_error_except};

    using namespace testing;

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], 0, 0, 0, nullptr, 0, nullptr))
        .Times(2)
        .WillOnce(Return(-EACCES))
        .WillOnce(Return(-EPERM));

    EXPECT_NO_THROW({
        output.clear_crtc();
    });
    EXPECT_NO_THROW({
        output.clear_crtc();
    });
}

TEST_F(RealKMSOutputTest, clear_crtc_throws_if_drm_call_fails_for_non_permission_reason)
{
    mir::FatalErrorStrategy on_error{mir::fatal_error_except};

    using namespace testing;

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], 0, 0, 0, nullptr, 0, nullptr))
        .Times(1)
        .WillOnce(Return(-EINVAL));

    EXPECT_THROW({
        output.clear_crtc();
    }, std::runtime_error);
}

TEST_F(RealKMSOutputTest, drm_set_gamma)
{
    using namespace testing;

    uint32_t const fb_id{67};

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    mg::GammaCurves gamma{{1}, {2}, {3}};

    EXPECT_CALL(mock_drm, drmModeCrtcSetGamma(drm_fd, crtc_ids[0],
                                              gamma.red.size(),
                                              const_cast<uint16_t*>(gamma.red.data()),
                                              const_cast<uint16_t*>(gamma.green.data()),
                                              const_cast<uint16_t*>(gamma.blue.data())))
        .Times(1);

    append_fb_id(fb_id);

    auto fb = output.fb_for(fake_bo);

    EXPECT_TRUE(output.set_crtc(*fb));

    output.set_gamma(gamma);
}

TEST_F(RealKMSOutputTest, drm_set_gamma_failure_does_not_throw)
{   // Regression test for LP: #1638220
    using namespace testing;

    uint32_t const fb_id{67};

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{
        drm_fd,
        mg::kms::get_connector(drm_fd, connector_ids[0]),
        mt::fake_shared(mock_page_flipper)};

    mg::GammaCurves gamma{{1}, {2}, {3}};

    EXPECT_CALL(mock_drm, drmModeCrtcSetGamma(drm_fd, crtc_ids[0],
                                              gamma.red.size(),
                                              const_cast<uint16_t*>(gamma.red.data()),
                                              const_cast<uint16_t*>(gamma.green.data()),
                                              const_cast<uint16_t*>(gamma.blue.data())))
        .WillOnce(Return(-ENOSYS));

    append_fb_id(fb_id);

    auto fb = output.fb_for(fake_bo);

    EXPECT_TRUE(output.set_crtc(*fb));

    EXPECT_NO_THROW(output.set_gamma(gamma););
}
