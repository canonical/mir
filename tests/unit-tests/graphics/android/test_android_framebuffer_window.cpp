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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/android_framebuffer_window.h"

#include "mir_test/egl_mock.h"
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;

class AndroidTestFramebufferWindow : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        /* silence uninteresting warning messages */
        mock_egl.silence_uninteresting();

    }

    mga::AndroidFramebufferWindow fb_win; 
    mir::EglMock mock_egl;
};

TEST_F(AndroidTestFramebufferWindow, eglChooseConfig_attr_is_terminated_by_null)
{
    using namespace testing;
    
    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, _, _))
        .Times(AtLeast(0))
        .WillOnce(DoAll(
                SaveArg<1>(&attr),
                SetArgPointee<2>(mock_egl.fake_configs),
                SetArgPointee<4>(mock_egl.fake_configs_num),
                Return(EGL_TRUE)));

    fb_win.android_display_egl_config(mock_egl.fake_egl_display);

    int i=0;
    while(attr[i++] != EGL_NONE);

    SUCCEED();
}

TEST_F(AndroidTestFramebufferWindow, eglChooseConfig_attr_contains_window_bit)
{
    using namespace testing;
    
    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, _, _))
        .Times(AtLeast(0))
        .WillOnce(DoAll(
                SaveArg<1>(&attr),
                SetArgPointee<2>(mock_egl.fake_configs),
                SetArgPointee<4>(mock_egl.fake_configs_num),
                Return(EGL_TRUE)));

    fb_win.android_display_egl_config(mock_egl.fake_egl_display);

    int i=0;
    bool validated = false;
    while(attr[i] != EGL_NONE)
    {
        if ((attr[i] == EGL_SURFACE_TYPE) && (attr[i+1] == EGL_WINDOW_BIT))
        {
            validated = true;
            break;
        }
        i++; 
    };

    EXPECT_TRUE(validated);
}

TEST_F(AndroidTestFramebufferWindow, eglChooseConfig_attr_requests_ogl2)
{
    using namespace testing;
    
    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, _, _))
        .Times(AtLeast(0))
        .WillOnce(DoAll(
                SaveArg<1>(&attr),
                SetArgPointee<2>(mock_egl.fake_configs),
                SetArgPointee<4>(mock_egl.fake_configs_num),
                Return(EGL_TRUE)));

    fb_win.android_display_egl_config(mock_egl.fake_egl_display);

    int i=0;
    bool validated = false;
    while(attr[i] != EGL_NONE)
    {
        if ((attr[i] == EGL_RENDERABLE_TYPE) && (attr[i+1] == EGL_OPENGL_ES2_BIT))
        {
            validated = true;
            break;
        } 
        i++; 
    };

    EXPECT_TRUE(validated);
}


#if 0

TEST_F(AndroidFramebufferWindow, queries_with_enough_room_for_all_potential_cfg)
{
    using namespace testing;

    int num_cfg = 43;
    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglGetConfigs(mock_egl.fake_egl_display, NULL, 0, _))
        .Times(AtLeast(1))
        .WillOnce(DoAll(
            SetArgPointee<3>(num_cfg),
            Return(EGL_TRUE)));

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, num_cfg, _))
        .Times(AtLeast(1))
        .WillOnce(DoAll(
                SaveArg<1>(&attr),
                SetArgPointee<2>(mock_egl.fake_configs),
                SetArgPointee<4>(mock_egl.fake_configs_num),
                Return(EGL_TRUE)));

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mga::AndroidDisplay(native_win));
    });

    /* should be able to ref this spot */
    EGLint test_last_spot = attr[num_cfg-1];
    EXPECT_EQ(test_last_spot, test_last_spot);

}
TEST_F(AndroidFramebufferWindow, creates_with_proper_visual_id)
{
    using namespace testing;
  
    EGLConfig cfg, chosen_cfg; 

    EXPECT_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display, _, EGL_NATIVE_VISUAL_ID, _))
        .Times(AtLeast(1))
        .WillRepeatedly(DoAll(
            SetArgPointee<3>(native_win->fake_visual_id),
            SaveArg<1>(&cfg),
            Return(EGL_TRUE)));

    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, _))
        .Times(Exactly(1))
        .WillOnce(DoAll(
            SaveArg<1>(&chosen_cfg),
            Return((EGLSurface)NULL)));

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mga::AndroidDisplay(native_win));
    });

    EXPECT_EQ(cfg, chosen_cfg);
}

TEST_F(AndroidFramebufferWindow, creates_with_proper_visual_id_mixed_valid_invalid)
{
    using namespace testing;
  
    EGLConfig cfg, chosen_cfg; 
    
    int bad_id = native_win->fake_visual_id + 1;

    EXPECT_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display, _, EGL_NATIVE_VISUAL_ID, _))
        .Times(AtLeast(1))
        .WillOnce(DoAll(
            SetArgPointee<3>(bad_id),
            Return(EGL_TRUE)))
        .WillOnce(DoAll(
            SetArgPointee<3>(native_win->fake_visual_id),
            SaveArg<1>(&cfg),
            Return(EGL_TRUE)))
        .WillRepeatedly(DoAll(
            SetArgPointee<3>(bad_id),
            Return(EGL_TRUE)));

    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, _))
        .Times(Exactly(1))
        .WillOnce(DoAll(
            SaveArg<1>(&chosen_cfg),
            Return((EGLSurface)NULL)));

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mga::AndroidDisplay(native_win));
    });

    EXPECT_EQ(cfg, chosen_cfg);
}

TEST_F(AndroidFramebufferWindow, without_proper_visual_id_throws)
{
    using namespace testing;
  
    EGLConfig cfg; 

    int bad_id = native_win->fake_visual_id + 1;
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display, _, EGL_NATIVE_VISUAL_ID, _))
        .Times(AtLeast(1))
        .WillRepeatedly(DoAll(
            SetArgPointee<3>(bad_id),
            SaveArg<1>(&cfg),
            Return(EGL_TRUE)));

    EXPECT_THROW(
    {
       std::shared_ptr<mg::Display> display(new mga::AndroidDisplay(native_win));
    }, std::runtime_error );
}
#endif
