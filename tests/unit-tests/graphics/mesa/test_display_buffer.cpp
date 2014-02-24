/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */
#include "src/platform/graphics/mesa/platform.h"
#include "src/platform/graphics/mesa/display_buffer.h"
#include "src/server/report/null_report_factory.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/null_virtual_terminal.h"
#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"
#include "mir_test_framework/udev_environment.h"
#include "mock_kms_output.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gbm.h>

using namespace testing;
using namespace mir;
using namespace std;
using namespace mir::test;
using namespace mir::test::doubles;
using namespace mir::mir_test_framework;
using namespace mir::graphics;
using mir::report::null_display_report;

class MesaDisplayBufferTest : public Test
{
public:
    MesaDisplayBufferTest()
    {
        ON_CALL(mock_egl, eglChooseConfig(_,_,_,1,_))
            .WillByDefault(DoAll(SetArgPointee<2>(mock_egl.fake_configs[0]),
                                 SetArgPointee<4>(1),
                                 Return(EGL_TRUE)));

        ON_CALL(mock_egl, eglQueryString(_,EGL_EXTENSIONS))
            .WillByDefault(Return("EGL_KHR_image "
                                  "EGL_KHR_image_base "
                                  "EGL_MESA_drm_image"));

        ON_CALL(mock_gl, glGetString(GL_EXTENSIONS))
            .WillByDefault(Return(reinterpret_cast<const GLubyte*>(
                                  "GL_OES_texture_npot "
                                  "GL_OES_EGL_image")));

        fake_bo = reinterpret_cast<gbm_bo*>(123);
        ON_CALL(mock_gbm, gbm_surface_lock_front_buffer(_))
            .WillByDefault(Return(fake_bo));
        fake_handle.u32 = 123;
        ON_CALL(mock_gbm, gbm_bo_get_handle(_))
            .WillByDefault(Return(fake_handle));
        ON_CALL(mock_gbm, gbm_bo_get_stride(_))
            .WillByDefault(Return(456));

        fake_devices.add_standard_device("standard-drm-devices");

        mock_kms_output = std::make_shared<NiceMock<MockKMSOutput>>();
        ON_CALL(*mock_kms_output, set_crtc(_))
            .WillByDefault(Return(true));
        ON_CALL(*mock_kms_output, schedule_page_flip(_))
            .WillByDefault(Return(true));
    }

    // The platform has an implicit dependency on mock_gbm etc so must be
    // reconstructed locally to ensure its lifetime is shorter than mock_gbm.
    shared_ptr<graphics::mesa::Platform> create_platform()
    {
        return make_shared<graphics::mesa::Platform>(
                      null_display_report(),
                      make_shared<NullVirtualTerminal>());
    }

protected:
    NiceMock<MockGBM> mock_gbm;
    NiceMock<MockEGL> mock_egl;
    NiceMock<MockGL>  mock_gl;
    NiceMock<MockDRM> mock_drm; 
    gbm_bo*           fake_bo;
    gbm_bo_handle     fake_handle;
    UdevEnvironment   fake_devices;
    std::shared_ptr<MockKMSOutput> mock_kms_output;
};

TEST_F(MesaDisplayBufferTest, unrotated_view_area_is_untouched)
{
    geometry::Rectangle const area{{12,34}, {56,78}};

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {},
        nullptr,
        area,
        mir_orientation_normal,
        mock_egl.fake_egl_context);

    EXPECT_EQ(area, db.view_area());
}

TEST_F(MesaDisplayBufferTest, normal_orientation_can_bypass)
{
    geometry::Rectangle const area{{12,34}, {56,78}};

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {},
        nullptr,
        area,
        mir_orientation_normal,
        mock_egl.fake_egl_context);

    EXPECT_TRUE(db.can_bypass());
}

TEST_F(MesaDisplayBufferTest, rotated_cannot_bypass)
{
    geometry::Rectangle const area{{12,34}, {56,78}};

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {},
        nullptr,
        area,
        mir_orientation_right,
        mock_egl.fake_egl_context);

    EXPECT_FALSE(db.can_bypass());
}

TEST_F(MesaDisplayBufferTest, orientation_not_implemented_internally)
{
    geometry::Rectangle const area{{12,34}, {56,78}};

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {},
        nullptr,
        area,
        mir_orientation_left,
        mock_egl.fake_egl_context);

    EXPECT_EQ(mir_orientation_left, db.orientation());
}

TEST_F(MesaDisplayBufferTest, normal_rotation_constructs_normal_fb)
{
    int const width = 56;
    int const height = 78;
    geometry::Rectangle const area{{12,34}, {width,height}};

    EXPECT_CALL(mock_gbm, gbm_bo_get_user_data(_))
        .WillOnce(Return((void*)0));
    EXPECT_CALL(mock_drm, drmModeAddFB(_, width, height, _, _, _, _, _))
        .Times(1);

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {},
        nullptr,
        area,
        mir_orientation_normal,
        mock_egl.fake_egl_context);
}

TEST_F(MesaDisplayBufferTest, left_rotation_constructs_transposed_fb)
{
    int const width = 56;
    int const height = 78;
    geometry::Rectangle const area{{12,34}, {width,height}};

    EXPECT_CALL(mock_gbm, gbm_bo_get_user_data(_))
        .WillOnce(Return((void*)0));
    EXPECT_CALL(mock_drm, drmModeAddFB(_, height, width, _, _, _, _, _))
        .Times(1);

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {},
        nullptr,
        area,
        mir_orientation_left,
        mock_egl.fake_egl_context);
}

TEST_F(MesaDisplayBufferTest, inverted_rotation_constructs_normal_fb)
{
    int const width = 56;
    int const height = 78;
    geometry::Rectangle const area{{12,34}, {width,height}};

    EXPECT_CALL(mock_gbm, gbm_bo_get_user_data(_))
        .WillOnce(Return((void*)0));
    EXPECT_CALL(mock_drm, drmModeAddFB(_, width, height, _, _, _, _, _))
        .Times(1);

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {},
        nullptr,
        area,
        mir_orientation_inverted,
        mock_egl.fake_egl_context);
}

TEST_F(MesaDisplayBufferTest, right_rotation_constructs_transposed_fb)
{
    int const width = 56;
    int const height = 78;
    geometry::Rectangle const area{{12,34}, {width,height}};

    EXPECT_CALL(mock_gbm, gbm_bo_get_user_data(_))
        .WillOnce(Return((void*)0));
    EXPECT_CALL(mock_drm, drmModeAddFB(_, height, width, _, _, _, _, _))
        .Times(1);

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {},
        nullptr,
        area,
        mir_orientation_right,
        mock_egl.fake_egl_context);
}

TEST_F(MesaDisplayBufferTest, first_post_flips_but_no_wait)
{
    geometry::Rectangle const area{{12,34}, {56,78}};

    EXPECT_CALL(*mock_kms_output, schedule_page_flip(_))
        .Times(1);
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(0);

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {mock_kms_output},
        nullptr,
        area,
        mir_orientation_normal,
        mock_egl.fake_egl_context);

    db.post_update();
}

TEST_F(MesaDisplayBufferTest, waits_for_page_flip_on_second_post)
{
    geometry::Rectangle const area{{12,34}, {56,78}};

    InSequence seq;

    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(0);
    EXPECT_CALL(*mock_kms_output, schedule_page_flip(_))
        .Times(1);
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(1);
    EXPECT_CALL(*mock_kms_output, schedule_page_flip(_))
        .Times(1);
    EXPECT_CALL(*mock_kms_output, wait_for_page_flip())
        .Times(0);

    graphics::mesa::DisplayBuffer db(
        create_platform(),
        null_display_report(),
        {mock_kms_output},
        nullptr,
        area,
        mir_orientation_normal,
        mock_egl.fake_egl_context);

    db.post_update();
    db.post_update();
}

