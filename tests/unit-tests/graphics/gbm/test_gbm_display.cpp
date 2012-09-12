/*
 * Copyright © 2012 Canonical Ltd.
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
#include "mir/graphics/gbm/gbm_display.h"
#include "mir_test/egl_mock.h"
#include "mock_drm.h"
#include "mock_gbm.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mgg=mir::graphics::gbm;

class GBMDisplayTest : public ::testing::Test
{
public:
    GBMDisplayTest()
    {
        using namespace testing;
        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
        .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                             SetArgPointee<4>(1),
                             Return(EGL_TRUE)));
    }


    void setup_post_update_expectations()
    {
        using namespace testing;

        EXPECT_CALL(mock_egl, eglSwapBuffers(mock_egl.fake_egl_display,
                                             mock_egl.fake_egl_surface))
            .Times(Exactly(2));

        EXPECT_CALL(mock_gbm, gbm_surface_lock_front_buffer(mock_gbm.fake_gbm.surface))
            .Times(Exactly(2))
            .WillOnce(Return(fake.bo1))
            .WillOnce(Return(fake.bo2));

        EXPECT_CALL(mock_gbm, gbm_bo_get_handle(fake.bo1))
            .Times(Exactly(1))
            .WillOnce(Return(fake.bo_handle1));

        EXPECT_CALL(mock_gbm, gbm_bo_get_handle(fake.bo2))
            .Times(Exactly(1))
            .WillOnce(Return(fake.bo_handle2));

        EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd,
                                           _, _, _, _, _,
                                           fake.bo_handle1.u32, _))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<7>(fake.fb_id1), Return(0)));

        EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd,
                                           _, _, _, _, _,
                                           fake.bo_handle2.u32, _))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<7>(fake.fb_id2), Return(0)));
    }

    struct FakeData {
        FakeData()
            : bo1{reinterpret_cast<gbm_bo*>(0xabcd)},
              bo2{reinterpret_cast<gbm_bo*>(0xabce)},
              fb_id1{66}, fb_id2{67}, crtc()
        {
            bo_handle1.u32 = 0x1234;
            bo_handle2.u32 = 0x1235;
            crtc.buffer_id = 88;
            crtc.crtc_id = 565;
        }

        gbm_bo* bo1;
        gbm_bo* bo2;
        uint32_t fb_id1;
        uint32_t fb_id2;
        gbm_bo_handle bo_handle1;
        gbm_bo_handle bo_handle2;
        drmModeCrtc crtc;
    } fake;

    ::testing::NiceMock<mir::EglMock> mock_egl;
    ::testing::NiceMock<mgg::MockDRM> mock_drm;
    ::testing::NiceMock<mgg::MockGBM> mock_gbm;
};

TEST_F(GBMDisplayTest, create_display)
{
    using namespace testing;

    /* To display a gbm surface, the GBMDisplay should... */

    /* Create a gbm surface to use as the frame buffer */
    EXPECT_CALL(mock_gbm, gbm_surface_create(mock_gbm.fake_gbm.device,_,_,_,_))
        .Times(Exactly(1));

    /* Create an EGL window surface backed by the gbm surface */
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display,
                                                 mock_egl.fake_configs[0],
                                                 (EGLNativeWindowType)mock_gbm.fake_gbm.surface, _))
        .Times(Exactly(1));

    /* Swap the EGL window surface to bring the back buffer to the front */
    EXPECT_CALL(mock_egl, eglSwapBuffers(mock_egl.fake_egl_display,
                                         mock_egl.fake_egl_surface))
        .Times(Exactly(1));

    /* Get the gbm_bo object corresponding to the front buffer */
    EXPECT_CALL(mock_gbm, gbm_surface_lock_front_buffer(mock_gbm.fake_gbm.surface))
        .Times(Exactly(1))
        .WillOnce(Return(fake.bo1));

    /* Get the DRM buffer handle associated with the gbm_bo */
    EXPECT_CALL(mock_gbm, gbm_bo_get_handle(fake.bo1))
        .Times(Exactly(1))
        .WillOnce(Return(fake.bo_handle1));

    /* Create a a DRM FB with the DRM buffer attached */
    EXPECT_CALL(mock_drm, drmModeAddFB(mock_drm.fake_drm.fd,
                                       _, _, _, _, _,
                                       fake.bo_handle1.u32, _))
        .Times(Exactly(1))
        .WillOnce(DoAll(SetArgPointee<7>(fake.fb_id1), Return(0)));

    /* Display the DRM FB (first expectation is for cleanup) */
    EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd, 
                                         mock_drm.fake_drm.encoders[1].crtc_id, Ne(fake.fb_id1),
                                         _, _,
                                         &mock_drm.fake_drm.connectors[1].connector_id,
                                         _, _))
        .Times(AtLeast(0));

    EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd, 
                                         mock_drm.fake_drm.encoders[1].crtc_id, fake.fb_id1,
                                         _, _,
                                         &mock_drm.fake_drm.connectors[1].connector_id,
                                         _, _))
        .Times(Exactly(1))
        .WillOnce(Return(0));


    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mgg::GBMDisplay>();
    });
}

TEST_F(GBMDisplayTest, reset_crtc_on_destruction)
{
    using namespace testing;

    {
        InSequence s;
        EXPECT_CALL(mock_drm, drmModeGetCrtc(mock_drm.fake_drm.fd,
                                             mock_drm.fake_drm.encoders[1].crtc_id))
            .Times(Exactly(1))
            .WillOnce(Return(&fake.crtc));

        /* 
         * Workaround: We use _ for the bufferId, instead of the more strict
         * Ne(fake.crtc.buffer_id), because using the latter causes valgrind
         * to report inexplicable uninitialized value errors.
         */
        EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd, 
                                             _, _, /* Ne(fake.crtc.buffer_id), */
                                             _, _,
                                             &mock_drm.fake_drm.connectors[1].connector_id,
                                             _, _))
            .Times(AtLeast(0));

        EXPECT_CALL(mock_drm, drmModeSetCrtc(mock_drm.fake_drm.fd, 
                                             fake.crtc.crtc_id, fake.crtc.buffer_id,
                                             _, _,
                                             &mock_drm.fake_drm.connectors[1].connector_id,
                                             _, _))
            .Times(Exactly(1));
    }

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mgg::GBMDisplay>();
    });
}

TEST_F(GBMDisplayTest, create_display_drm_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_drm, drmOpen(_,_))
        .Times(Exactly(1))
        .WillOnce(Return(-1));

    EXPECT_CALL(mock_drm, drmClose(_))
        .Times(Exactly(0));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mgg::GBMDisplay>();
    }, std::runtime_error);
}

TEST_F(GBMDisplayTest, create_display_kms_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_drm, drmModeGetResources(_))
        .Times(Exactly(1))
        .WillOnce(Return(reinterpret_cast<drmModeRes*>(0)));

    EXPECT_CALL(mock_drm, drmModeFreeResources(_))
        .Times(Exactly(0));

    EXPECT_CALL(mock_drm, drmClose(_))
        .Times(Exactly(1));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mgg::GBMDisplay>();
    }, std::runtime_error);
}

TEST_F(GBMDisplayTest, create_display_gbm_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_gbm, gbm_create_device(_))
        .Times(Exactly(1))
        .WillOnce(Return(reinterpret_cast<gbm_device*>(0)));

    EXPECT_CALL(mock_gbm, gbm_device_destroy(_))
        .Times(Exactly(0));

    {
        InSequence s;

        EXPECT_CALL(mock_drm, drmModeFreeResources(_))
            .Times(Exactly(1));

        EXPECT_CALL(mock_drm, drmClose(_))
            .Times(Exactly(1));
    }

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mgg::GBMDisplay>();
    }, std::runtime_error);
}

TEST_F(GBMDisplayTest, post_update)
{
    using namespace testing;

    setup_post_update_expectations();

    {
        InSequence s;

        /* Flip the new FB */
        EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd, 
                                              mock_drm.fake_drm.encoders[1].crtc_id,
                                              fake.fb_id2,
                                              _, _))
            .Times(Exactly(1))
            .WillOnce(Return(0));

        /* Release the current FB */
        EXPECT_CALL(mock_gbm, gbm_surface_release_buffer(mock_gbm.fake_gbm.surface, fake.bo1))
            .Times(Exactly(1));

        /* Release the new FB (at destruction time) */
        EXPECT_CALL(mock_gbm, gbm_surface_release_buffer(mock_gbm.fake_gbm.surface, fake.bo2))
            .Times(Exactly(1));
    }


    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mgg::GBMDisplay>();
        EXPECT_TRUE(display->post_update());
    });
}

TEST_F(GBMDisplayTest, post_update_flip_failure)
{
    using namespace testing;

    setup_post_update_expectations();

    {
        InSequence s;

        /* New FB flip failure */
        EXPECT_CALL(mock_drm, drmModePageFlip(mock_drm.fake_drm.fd, 
                                              mock_drm.fake_drm.encoders[1].crtc_id,
                                              fake.fb_id2,
                                              _, _))
            .Times(Exactly(1))
            .WillOnce(Return(-1));

        /* Release the new (not flipped) BO */
        EXPECT_CALL(mock_gbm, gbm_surface_release_buffer(mock_gbm.fake_gbm.surface, fake.bo2))
            .Times(Exactly(1));

        /* Release the current FB (at destruction time) */
        EXPECT_CALL(mock_gbm, gbm_surface_release_buffer(mock_gbm.fake_gbm.surface, fake.bo1))
            .Times(Exactly(1));
    }

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mgg::GBMDisplay>();
        EXPECT_FALSE(display->post_update());
    });
}
