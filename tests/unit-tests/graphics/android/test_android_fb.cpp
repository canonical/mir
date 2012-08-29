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

TEST(framebuffer_test, fb_initialize_GetDisplay)
{
    using namespace testing;

    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
            .Times(Exactly(1));

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay);
    });
}

TEST(framebuffer_test, fb_initialize_Initialize)
{
    using namespace testing;

    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
            .Times(Exactly(1));

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay);
    });
}

TEST(framebuffer_test, fb_initialize_display_failure)
{
    using namespace testing;

    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
            .Times(Exactly(1))
            .WillOnce(Return((EGLDisplay)EGL_NO_DISPLAY));

    EXPECT_THROW(
    {
       std::shared_ptr<mg::Display> display(new mg::AndroidDisplay);
    }, std::runtime_error   );
}

TEST(framebuffer_test, fb_initialize_Initialize_failure)
{
    using namespace testing;

    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
            .Times(Exactly(1))
            .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
       std::shared_ptr<mg::Display> display(new mg::AndroidDisplay);
    }, std::runtime_error   );
}

TEST(framebuffer_test, fb_initialize_Initialize_version_mismatch)
{
    using namespace testing;

    mir::EglMock mock_egl;
    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
            .Times(Exactly(1))
            .WillOnce(DoAll(
                    SetArgPointee<2>(2),
                    Return(EGL_FALSE)));

    EXPECT_THROW(
    {
       std::shared_ptr<mg::Display> display(new mg::AndroidDisplay);
    }, std::runtime_error   );
}

TEST(framebuffer_test, fb_initialize_configure_attr_is_terminated_by_null)
{
    using namespace testing;
    
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
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay);
    });

    int i=0;
    while(attr[i++] != EGL_NONE);

    SUCCEED();
}

#if 0
TEST(framebuffer_test, fb_initialize_configure_attr_contains_win_type)
{
    using namespace testing;
    
    mir::EglMock mock_egl;
    const EGLint *attr;
    EGLint num;

    EXPECT_CALL(mock_egl, eglChooseConfig(mock_egl.fake_egl_display, _, _, _, _))
        .Times(AtLeast(0))
        .WillOnce(DoAll(
                SaveArg<1>(&attr),
                SaveArgPointee<5>(&num),
                SetArgPointee<2>(&mock_egl.fake_configs),
                SetArgPointee<4>(mock_egl.fake_configs_num),
                Return(EGL_TRUE)));

    EXPECT_NO_THROW({
    std::shared_ptr<mg::Display> display(new mg::AndroidDisplay);
    });

    EXPECT_EQ(attr[num-1], EGL_NONE);
}
#endif
