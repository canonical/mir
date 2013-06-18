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

#include "src/server/graphics/android/android_framebuffer_window.h"

#include "mir_test_doubles/mock_egl.h"

#include <memory>
#include <system/window.h>
#include <gtest/gtest.h>
#include <stdexcept>

namespace mga = mir::graphics::android;
namespace mtd = mir::test::doubles;

class ANativeWindowInterface
{
public:
    virtual ~ANativeWindowInterface() = default;
    virtual int query_interface(const ANativeWindow* win , int code, int* value) const = 0;
};

class MockANativeWindow : public ANativeWindowInterface,
    public ANativeWindow
{
public:
    MockANativeWindow()
        : fake_visual_id(5)
    {
        using namespace testing;

        query = hook_query;

        ON_CALL(*this, query_interface(_,_,_))
        .WillByDefault(DoAll(
                           SetArgPointee<2>(fake_visual_id),
                           Return(0)));
    }

    ~MockANativeWindow() noexcept {}

    static int hook_query(const ANativeWindow* anw, int code, int *ret)
    {
        const MockANativeWindow* mocker = static_cast<const MockANativeWindow*>(anw);
        return mocker->query_interface(anw, code, ret);
    }

    MOCK_CONST_METHOD3(query_interface,int(const ANativeWindow*,int,int*));

    int fake_visual_id;
};

class AndroidFramebufferWindowConfigSelection : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        /* silence uninteresting warning messages */
        mock_egl.silence_uninteresting();

        mock_anw = std::make_shared<MockANativeWindow>();
        fb_win = std::make_shared<mga::AndroidFramebufferWindow>(mock_anw);

        EXPECT_CALL(*mock_anw, query_interface(_,_,_))
        .Times(AtLeast(0));
    }

    std::shared_ptr<MockANativeWindow> mock_anw;
    std::shared_ptr<mga::AndroidFramebufferWindow> fb_win;
    mtd::MockEGL mock_egl;
};

TEST_F(AndroidFramebufferWindowConfigSelection, queries_for_native_visual_id)
{
    using namespace testing;
    EXPECT_CALL(*mock_anw, query_interface(_,NATIVE_WINDOW_FORMAT,_))
    .Times(AtLeast(1));

    fb_win->android_display_egl_config(mock_egl.fake_egl_display);
}

TEST_F(AndroidFramebufferWindowConfigSelection, eglChooseConfig_attr_is_terminated_by_null)
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

    fb_win->android_display_egl_config(mock_egl.fake_egl_display);

    int i=0;
    while(attr[i++] != EGL_NONE);

    SUCCEED();
}

TEST_F(AndroidFramebufferWindowConfigSelection, eglChooseConfig_attr_contains_window_bit)
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

    fb_win->android_display_egl_config(mock_egl.fake_egl_display);

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

TEST_F(AndroidFramebufferWindowConfigSelection, eglChooseConfig_attr_requests_ogl2)
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

    fb_win->android_display_egl_config(mock_egl.fake_egl_display);

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

TEST_F(AndroidFramebufferWindowConfigSelection, queries_with_enough_room_for_all_potential_cfg)
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

    fb_win->android_display_egl_config(mock_egl.fake_egl_display);

    /* should be able to ref this spot */
    EGLint test_last_spot = attr[num_cfg-1];
    EXPECT_EQ(test_last_spot, test_last_spot);

}

TEST_F(AndroidFramebufferWindowConfigSelection, creates_with_proper_visual_id)
{
    using namespace testing;

    EGLConfig cfg, chosen_cfg;

    EXPECT_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display, _, EGL_NATIVE_VISUAL_ID, _))
    .Times(AtLeast(1))
    .WillRepeatedly(DoAll(
                        SetArgPointee<3>(mock_anw->fake_visual_id),
                        SaveArg<1>(&cfg),
                        Return(EGL_TRUE)));

    chosen_cfg = fb_win->android_display_egl_config(mock_egl.fake_egl_display);

    EXPECT_EQ(cfg, chosen_cfg);
}

TEST_F(AndroidFramebufferWindowConfigSelection, creates_with_proper_visual_id_mixed_valid_invalid)
{
    using namespace testing;

    EGLConfig cfg, chosen_cfg;

    int bad_id = mock_anw->fake_visual_id + 1;

    EXPECT_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display, _, EGL_NATIVE_VISUAL_ID, _))
    .Times(AtLeast(1))
    .WillOnce(DoAll(
                  SetArgPointee<3>(bad_id),
                  Return(EGL_TRUE)))
    .WillOnce(DoAll(
                  SetArgPointee<3>(mock_anw->fake_visual_id),
                  SaveArg<1>(&cfg),
                  Return(EGL_TRUE)))
    .WillRepeatedly(DoAll(
                        SetArgPointee<3>(bad_id),
                        Return(EGL_TRUE)));

    chosen_cfg = fb_win->android_display_egl_config(mock_egl.fake_egl_display);

    EXPECT_EQ(cfg, chosen_cfg);
}

TEST_F(AndroidFramebufferWindowConfigSelection, without_proper_visual_id_throws)
{
    using namespace testing;

    EGLConfig cfg;

    int bad_id = mock_anw->fake_visual_id + 1;
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(mock_egl.fake_egl_display, _, EGL_NATIVE_VISUAL_ID, _))
    .Times(AtLeast(1))
    .WillRepeatedly(DoAll(
                        SetArgPointee<3>(bad_id),
                        SaveArg<1>(&cfg),
                        Return(EGL_TRUE)));

    EXPECT_THROW(
    {
        fb_win->android_display_egl_config(mock_egl.fake_egl_display);
    }, std::runtime_error );
}

TEST_F(AndroidFramebufferWindowConfigSelection, anw_is_not_null)
{
    using namespace testing;

    auto win_handle = fb_win->android_native_window_type();

    EXPECT_NE(win_handle, (EGLNativeWindowType) NULL);
}
