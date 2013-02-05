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

#include "src/graphics/gbm/kms_page_flip_manager.h"

#include "mock_drm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mgg = mir::graphics::gbm;

namespace
{

class KMSPageFlipManagerTest : public ::testing::Test
{
public:
    KMSPageFlipManagerTest()
        : pf_manager{mock_drm.fake_drm.fd(), std::chrono::milliseconds{50}},
          blocking_pf_manager{mock_drm.fake_drm.fd(), std::chrono::hours{1}}
    {
    }

    testing::NiceMock<mgg::MockDRM> mock_drm;
    mgg::KMSPageFlipManager pf_manager;
    mgg::KMSPageFlipManager blocking_pf_manager;
};

ACTION_P(InvokePageFlipHandler, param)
{
    int const dont_care{0};
    char dummy;

    arg1->page_flip_handler(dont_care, dont_care, dont_care, dont_care, *param);
    ASSERT_EQ(1, read(arg0, &dummy, 1));
}

}

TEST_F(KMSPageFlipManagerTest, schedule_page_flip_calls_drm_page_flip)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1);

    pf_manager.schedule_page_flip(crtc_id, fb_id);
}

TEST_F(KMSPageFlipManagerTest, wait_for_page_flip_handles_drm_event)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};
    void* user_data{nullptr};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1)
        .WillOnce(DoAll(SaveArg<4>(&user_data), Return(0)));

    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(1)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data), Return(0)));

    pf_manager.schedule_page_flip(crtc_id, fb_id);

    /* Fake a DRM event */
    EXPECT_EQ(1, write(mock_drm.fake_drm.write_fd(), "a", 1));

    pf_manager.wait_for_page_flip(crtc_id);
}

TEST_F(KMSPageFlipManagerTest, wait_for_page_flip_doesnt_block_forever_if_no_drm_event_comes)
{
    using namespace testing;

    uint32_t const crtc_id{10};
    uint32_t const fb_id{101};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          crtc_id, fb_id, _, _))
        .Times(1);

    EXPECT_CALL(mock_drm, drmHandleEvent(_, _))
        .Times(0);

    pf_manager.schedule_page_flip(crtc_id, fb_id);

    /* No DRM event, should time out */

    pf_manager.wait_for_page_flip(crtc_id);
}

TEST_F(KMSPageFlipManagerTest, wait_for_non_scheduled_page_flip_doesnt_block)
{
    using namespace testing;

    uint32_t const crtc_id{10};

    EXPECT_CALL(mock_drm, drmModePageFlip(_, _, _, _, _))
        .Times(0);

    EXPECT_CALL(mock_drm, drmHandleEvent(_, _))
        .Times(0);

    blocking_pf_manager.wait_for_page_flip(crtc_id);
}

TEST_F(KMSPageFlipManagerTest, wait_for_page_flips_interleaved)
{
    using namespace testing;

    uint32_t const fb_id{101};
    std::vector<uint32_t> const crtc_ids{10, 11, 12};
    std::vector<void*> user_data{nullptr, nullptr, nullptr};

    EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd(),
                                          _, fb_id, _, _))
        .Times(3)
        .WillOnce(DoAll(SaveArg<4>(&user_data[0]), Return(0)))
        .WillOnce(DoAll(SaveArg<4>(&user_data[1]), Return(0)))
        .WillOnce(DoAll(SaveArg<4>(&user_data[2]), Return(0)));

    EXPECT_CALL(mock_drm, drmHandleEvent(mock_drm.fake_drm.fd(), _))
        .Times(3)
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[1]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[2]), Return(0)))
        .WillOnce(DoAll(InvokePageFlipHandler(&user_data[0]), Return(0)));

    for (auto crtc_id : crtc_ids)
        pf_manager.schedule_page_flip(crtc_id, fb_id);

    /* Fake 3 DRM event */
    EXPECT_EQ(3, write(mock_drm.fake_drm.write_fd(), "abc", 3));

    for (auto crtc_id : crtc_ids)
        pf_manager.wait_for_page_flip(crtc_id);
}
