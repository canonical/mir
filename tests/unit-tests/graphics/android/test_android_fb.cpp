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

#include "mir/graphics/android/android_display.h"
#include "mir_test/egl_mock.h"
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
namespace mg=mir::graphics;

class AndroidFramebufferWindow 
{
    virtual int getAndroidVisualId() = 0;
};

class MockAndroidFramebufferWindow : public AndroidFramebufferWindow
{
public:
    MockAndroidFramebufferWindow() {};
    ~MockAndroidFramebufferWindow() {};

    MOCK_METHOD0(getAndroidVisualId, int());

};

TEST(framebuffer_test, fb_initialize_GetDisplay)
{
    using namespace testing;

    std::shared_ptr<AndroidFramebufferWindow> native_win(new MockAndroidFramebufferWindow);
    mir::EglMock mock_egl;

    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
            .Times(Exactly(1));

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    });
}

TEST(framebuffer_test, fb_initialize_Initialize)
{
    using namespace testing;

    MockAndroidFramebufferWindow win;
    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
            .Times(Exactly(1));

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    });
}

TEST(framebuffer_test, fb_initialize_display_failure)
{
    using namespace testing;

    MockAndroidFramebufferWindow win;
    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
            .Times(Exactly(1))
            .WillOnce(Return((EGLDisplay)EGL_NO_DISPLAY));

    EXPECT_THROW(
    {
       std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    }, std::runtime_error   );
}

TEST(framebuffer_test, fb_initialize_Initialize_failure)
{
    using namespace testing;

    MockAndroidFramebufferWindow win;
    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
            .Times(Exactly(1))
            .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
       std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    }, std::runtime_error   );
}

TEST(framebuffer_test, fb_initialize_Initialize_version_mismatch)
{
    using namespace testing;

    MockAndroidFramebufferWindow win;
    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
            .Times(Exactly(1))
            .WillOnce(DoAll(
                    SetArgPointee<2>(2),
                    Return(EGL_FALSE)));

    EXPECT_THROW(
    {
       std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    }, std::runtime_error   );
}

TEST(framebuffer_test, fb_initialize_configure_attr_is_terminated_by_null)
{
    using namespace testing;
    
    MockAndroidFramebufferWindow win;
    mir::EglMock mock_egl;
    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, _, _))
        .Times(AtLeast(0))
        .WillOnce(DoAll(
                SaveArg<1>(&attr),
                SetArgPointee<2>(mock_egl.fake_configs),
                SetArgPointee<4>(mock_egl.fake_configs_num),
                Return(EGL_TRUE)));
    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    });

    int i=0;
    while(attr[i++] != EGL_NONE);

    SUCCEED();
}

TEST(framebuffer_test, fb_initialize_configure_attr_contains_window_bit)
{
    using namespace testing;
    
    MockAndroidFramebufferWindow win;
    mir::EglMock mock_egl;
    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, _, _))
        .Times(AtLeast(0))
        .WillOnce(DoAll(
                SaveArg<1>(&attr),
                SetArgPointee<2>(mock_egl.fake_configs),
                SetArgPointee<4>(mock_egl.fake_configs_num),
                Return(EGL_TRUE)));
    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    });

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

TEST(framebuffer_test, fb_initialize_configure_attr_requests_ogl2)
{
    using namespace testing;
    
    MockAndroidFramebufferWindow win;
    mir::EglMock mock_egl;
    const EGLint *attr;

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, _, _))
        .Times(AtLeast(0))
        .WillOnce(DoAll(
                SaveArg<1>(&attr),
                SetArgPointee<2>(mock_egl.fake_configs),
                SetArgPointee<4>(mock_egl.fake_configs_num),
                Return(EGL_TRUE)));
    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    });

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


TEST(framebuffer_test, fb_initialize_check_config_uses_proper_native_visual_id)
{
    using namespace testing;
    
    MockAndroidFramebufferWindow win;
    mir::EglMock mock_egl;

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay(native_win));
    });



}
