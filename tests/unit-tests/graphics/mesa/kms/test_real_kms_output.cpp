/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

#include "mir/test/fake_shared.h"

#include "mir/test/doubles/mock_drm.h"

#include <stdexcept>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

class NullPageFlipper : public mgm::PageFlipper
{
public:
    bool schedule_flip(uint32_t,uint32_t) { return true; }
    void wait_for_flip(uint32_t) { }
};

class MockPageFlipper : public mgm::PageFlipper
{
public:
    MOCK_METHOD2(schedule_flip, bool(uint32_t,uint32_t));
    MOCK_METHOD1(wait_for_flip, void(uint32_t));
};

class RealKMSOutputTest : public ::testing::Test
{
public:
    RealKMSOutputTest()
        : invalid_id{0}, crtc_ids{10, 11},
          encoder_ids{20, 21}, connector_ids{30, 21},
          possible_encoder_ids1{encoder_ids[0]},
          possible_encoder_ids2{encoder_ids[0], encoder_ids[1]}
    {
    }

    void setup_outputs_connected_crtc()
    {
        mtd::FakeDRMResources& resources(mock_drm.fake_drm);
        uint32_t const possible_crtcs_mask{0x1};

        resources.reset();

        resources.add_crtc(crtc_ids[0], drmModeModeInfo());
        resources.add_encoder(encoder_ids[0], crtc_ids[0], possible_crtcs_mask);
        resources.add_connector(connector_ids[0], DRM_MODE_CONNECTOR_VGA,
                                DRM_MODE_CONNECTED, encoder_ids[0],
                                modes_empty, possible_encoder_ids1, geom::Size());

        resources.prepare();
    }

    void setup_outputs_no_connected_crtc()
    {
        mtd::FakeDRMResources& resources(mock_drm.fake_drm);
        uint32_t const possible_crtcs_mask1{0x1};
        uint32_t const possible_crtcs_mask_all{0x3};

        resources.reset();

        resources.add_crtc(crtc_ids[0], drmModeModeInfo());
        resources.add_crtc(crtc_ids[1], drmModeModeInfo());
        resources.add_encoder(encoder_ids[0], crtc_ids[0], possible_crtcs_mask1);
        resources.add_encoder(encoder_ids[1], invalid_id, possible_crtcs_mask_all);
        resources.add_connector(connector_ids[0], DRM_MODE_CONNECTOR_Composite,
                                DRM_MODE_CONNECTED, invalid_id,
                                modes_empty, possible_encoder_ids2, geom::Size());
        resources.add_connector(connector_ids[1], DRM_MODE_CONNECTOR_DVIA,
                                DRM_MODE_CONNECTED, encoder_ids[0],
                                modes_empty, possible_encoder_ids2, geom::Size());

        resources.prepare();
    }

    testing::NiceMock<mtd::MockDRM> mock_drm;
    MockPageFlipper mock_page_flipper;
    NullPageFlipper null_page_flipper;

    std::vector<drmModeModeInfo> modes_empty;
    uint32_t const invalid_id;
    std::vector<uint32_t> const crtc_ids;
    std::vector<uint32_t> const encoder_ids;
    std::vector<uint32_t> const connector_ids;
    std::vector<uint32_t> possible_encoder_ids1;
    std::vector<uint32_t> possible_encoder_ids2;
};

}

TEST_F(RealKMSOutputTest, construction_queries_connector)
{
    using namespace testing;

    setup_outputs_connected_crtc();

    EXPECT_CALL(mock_drm, drmModeGetConnector(_,connector_ids[0]))
        .Times(1);

    mgm::RealKMSOutput output{mock_drm.fake_drm.fd(), connector_ids[0],
                              mt::fake_shared(null_page_flipper)};
}

TEST_F(RealKMSOutputTest, operations_use_existing_crtc)
{
    using namespace testing;

    uint32_t const fb_id{67};

    setup_outputs_connected_crtc();

    {
        InSequence s;

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], fb_id, _, _,
                                             Pointee(connector_ids[0]), _, _))
            .Times(1);

        EXPECT_CALL(mock_page_flipper, schedule_flip(crtc_ids[0], fb_id))
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_CALL(mock_page_flipper, wait_for_flip(crtc_ids[0]))
            .Times(1);

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], Ne(fb_id), _, _,
                                             Pointee(connector_ids[0]), _, _))
            .Times(1);
    }

    mgm::RealKMSOutput output{mock_drm.fake_drm.fd(), connector_ids[0],
                              mt::fake_shared(mock_page_flipper)};

    EXPECT_TRUE(output.set_crtc(fb_id));
    EXPECT_TRUE(output.schedule_page_flip(fb_id));
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

        EXPECT_CALL(mock_page_flipper, schedule_flip(crtc_ids[1], fb_id))
            .Times(1)
            .WillOnce(Return(true));

        EXPECT_CALL(mock_page_flipper, wait_for_flip(crtc_ids[1]))
            .Times(1);

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, 0, 0, _, _,
                                             Pointee(connector_ids[0]), _, _))
            .Times(1);
    }

    mgm::RealKMSOutput output{mock_drm.fake_drm.fd(), connector_ids[0],
                              mt::fake_shared(mock_page_flipper)};

    EXPECT_TRUE(output.set_crtc(fb_id));
    EXPECT_TRUE(output.schedule_page_flip(fb_id));
    output.wait_for_page_flip();
}

TEST_F(RealKMSOutputTest, set_crtc_failure_is_handled_gracefully)
{
    using namespace testing;

    uint32_t const fb_id{67};

    setup_outputs_connected_crtc();

    {
        InSequence s;

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], fb_id, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(1));

        EXPECT_CALL(mock_page_flipper, schedule_flip(_, _))
            .Times(0);

        EXPECT_CALL(mock_page_flipper, wait_for_flip(_))
            .Times(0);

        EXPECT_CALL(mock_drm, drmModeSetCrtc(_, _, _, _, _, _, _, _))
            .Times(0);
    }

    mgm::RealKMSOutput output{mock_drm.fake_drm.fd(), connector_ids[0],
                              mt::fake_shared(mock_page_flipper)};

    EXPECT_FALSE(output.set_crtc(fb_id));
    EXPECT_THROW({
        output.schedule_page_flip(fb_id);
    }, std::runtime_error);
    EXPECT_THROW({
        output.wait_for_page_flip();
    }, std::runtime_error);
}

TEST_F(RealKMSOutputTest, clear_crtc_gets_crtc_if_none_is_current)
{
    using namespace testing;

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{mock_drm.fake_drm.fd(), connector_ids[0],
                              mt::fake_shared(mock_page_flipper)};

    EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], 0, 0, 0, nullptr, 0, nullptr))
        .Times(1)
        .WillOnce(Return(0));

    output.clear_crtc();
}

TEST_F(RealKMSOutputTest, clear_crtc_does_not_throw_if_no_crtc_is_found)
{
    using namespace testing;

    mtd::FakeDRMResources& resources(mock_drm.fake_drm);
    uint32_t const possible_crtcs_mask_empty{0x0};

    resources.reset();

    resources.add_encoder(encoder_ids[0], invalid_id, possible_crtcs_mask_empty);
    resources.add_connector(connector_ids[0], DRM_MODE_CONNECTOR_VGA,
                            DRM_MODE_CONNECTED, encoder_ids[0],
                            modes_empty, possible_encoder_ids1, geom::Size());

    resources.prepare();

    mgm::RealKMSOutput output{mock_drm.fake_drm.fd(), connector_ids[0],
                              mt::fake_shared(mock_page_flipper)};

    EXPECT_CALL(mock_drm, drmModeSetCrtc(_, _, 0, 0, 0, nullptr, 0, nullptr))
        .Times(0);

    output.clear_crtc();
}

TEST_F(RealKMSOutputTest, clear_crtc_throws_if_drm_call_fails)
{
    using namespace testing;

    setup_outputs_connected_crtc();

    mgm::RealKMSOutput output{mock_drm.fake_drm.fd(), connector_ids[0],
                              mt::fake_shared(mock_page_flipper)};

    EXPECT_CALL(mock_drm, drmModeSetCrtc(_, crtc_ids[0], 0, 0, 0, nullptr, 0, nullptr))
        .Times(1)
        .WillOnce(Return(-1));

    EXPECT_THROW({
        output.clear_crtc();
    }, std::runtime_error);
}
