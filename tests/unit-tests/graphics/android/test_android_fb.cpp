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

#include "src/server/graphics/android/android_display.h"
#include "src/server/graphics/android/android_framebuffer_window_query.h"
#include "mir_test/egl_mock.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

class MockAndroidFramebufferWindow : public mga::AndroidFramebufferWindowQuery
{
public:
    MockAndroidFramebufferWindow() {}
    ~MockAndroidFramebufferWindow() {}

    MOCK_CONST_METHOD0(android_native_window_type, EGLNativeWindowType());
    MOCK_CONST_METHOD1(android_display_egl_config, EGLConfig(EGLDisplay));
};

class AndroidTestFramebufferInit : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        native_win = std::make_shared<MockAndroidFramebufferWindow>();

        /* silence uninteresting warning messages */
        mock_egl.silence_uninteresting();

        EXPECT_CALL(*native_win, android_native_window_type())
        .Times(AtLeast(0));
        EXPECT_CALL(*native_win, android_display_egl_config(_))
        .Times(AtLeast(0));

        width = 435;
        height = 477;
    }

    std::shared_ptr<MockAndroidFramebufferWindow> native_win;
    mir::EglMock mock_egl;
    int width, height;
};

TEST_F(AndroidTestFramebufferInit, eglGetDisplay)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });
}

TEST_F(AndroidTestFramebufferInit, eglGetDisplay_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
    .Times(Exactly(1))
    .WillOnce(Return((EGLDisplay)EGL_NO_DISPLAY));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, eglInitialize)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });
}

TEST_F(AndroidTestFramebufferInit, eglInitialize_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, eglInitialize_failure_bad_major_version)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SetArgPointee<1>(2),
                  Return(EGL_FALSE)));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, eglInitialize_failure_bad_minor_version)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglInitialize(mock_egl.fake_egl_display, _, _))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SetArgPointee<2>(2),
                  Return(EGL_FALSE)));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, eglCreateWindowSurface_requests_config)
{
    using namespace testing;
    EGLConfig fake_config = (EGLConfig) 0x3432;
    EXPECT_CALL(*native_win, android_display_egl_config(_))
    .Times(Exactly(1))
    .WillOnce(Return(fake_config));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, fake_config, _, _))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });
}

TEST_F(AndroidTestFramebufferInit, eglCreateWindowSurface_nullarg)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, NULL))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });
}

TEST_F(AndroidTestFramebufferInit, eglCreateWindowSurface_uses_native_window_type)
{
    using namespace testing;
    EGLNativeWindowType egl_window = (EGLNativeWindowType)0x4443;

    EXPECT_CALL(*native_win, android_native_window_type())
    .Times(Exactly(1))
    .WillOnce(Return(egl_window));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, egl_window,_))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });
}

TEST_F(AndroidTestFramebufferInit, eglCreateWindowSurface_failure)
{
    using namespace testing;
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display,_,_,_))
    .Times(Exactly(1))
    .WillOnce(Return((EGLSurface)NULL));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    }, std::runtime_error);
}

/* create context stuff */
TEST_F(AndroidTestFramebufferInit, CreateContext_window_cfg_matches_context_cfg)
{
    using namespace testing;

    EGLConfig chosen_cfg, cfg;
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, _))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SaveArg<1>(&chosen_cfg),
                  Return((EGLSurface)mock_egl.fake_egl_surface)));

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _,_))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SaveArg<1>(&cfg),
                  Return((EGLContext)mock_egl.fake_egl_context)));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });

    EXPECT_EQ(chosen_cfg, cfg);
}

TEST_F(AndroidTestFramebufferInit, CreateContext_ensure_no_share)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, EGL_NO_CONTEXT,_))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });
}

TEST_F(AndroidTestFramebufferInit, CreateContext_context_attr_null_terminated)
{
    using namespace testing;

    const EGLint* context_attr;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SaveArg<3>(&context_attr),
                  Return((EGLContext)mock_egl.fake_egl_context)));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });

    int i=0;
    EXPECT_NO_THROW(
    {
        while(context_attr[i++] != EGL_NONE);
    });

    SUCCEED();
}

TEST_F(AndroidTestFramebufferInit, CreateContext_context_uses_client_version_2)
{
    using namespace testing;

    const EGLint* context_attr;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
    .Times(Exactly(1))
    .WillOnce(DoAll(
                  SaveArg<3>(&context_attr),
                  Return((EGLContext)mock_egl.fake_egl_context)));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });

    int i=0;
    bool validated = false;
    while(context_attr[i] != EGL_NONE)
    {
        if ((context_attr[i] == EGL_CONTEXT_CLIENT_VERSION) && (context_attr[i+1] == 2))
        {
            validated = true;
            break;
        }
        i++;
    };

    EXPECT_TRUE(validated);
}

TEST_F(AndroidTestFramebufferInit, CreateContext_failure)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
    .Times(Exactly(1))
    .WillOnce(Return((EGLContext)EGL_NO_CONTEXT));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    }, std::runtime_error   );
}

TEST_F(AndroidTestFramebufferInit, MakeCurrent_uses_correct_surface)
{
    using namespace testing;
    EGLSurface fake_surface = (EGLSurface) 0x715;

    EXPECT_CALL(mock_egl, eglCreateWindowSurface(mock_egl.fake_egl_display, _, _, _))
    .Times(Exactly(1))
    .WillOnce(Return(fake_surface));
    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, fake_surface, fake_surface, _))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });

}

TEST_F(AndroidTestFramebufferInit, MakeCurrent_uses_correct_context)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
    .Times(Exactly(1))
    .WillOnce(Return(mock_egl.fake_egl_context));

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, mock_egl.fake_egl_context))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });

}

TEST_F(AndroidTestFramebufferInit, eglMakeCurrent_failure_throws)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglMakeCurrent(mock_egl.fake_egl_display, _, _, _))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_FALSE));

    EXPECT_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    }, std::runtime_error);

}

TEST_F(AndroidTestFramebufferInit, eglContext_resource_freed)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglCreateContext(mock_egl.fake_egl_display, _, _, _ ))
    .Times(Exactly(1))
    .WillOnce(Return(mock_egl.fake_egl_context));

    EXPECT_CALL(mock_egl, eglDestroyContext(mock_egl.fake_egl_display, mock_egl.fake_egl_context))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });

}

TEST_F(AndroidTestFramebufferInit, eglSurface_resource_freed)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglDestroySurface(mock_egl.fake_egl_display, mock_egl.fake_egl_surface))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });

}

TEST_F(AndroidTestFramebufferInit, eglDisplay_is_terminated)
{
    using namespace testing;

    EXPECT_CALL(mock_egl, eglTerminate(mock_egl.fake_egl_display))
    .Times(Exactly(1));

    EXPECT_NO_THROW(
    {
        std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);
    });

}

TEST_F(AndroidTestFramebufferInit, display_post_calls_swapbuffers_once)
{
    using namespace testing;
    std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);

    EXPECT_CALL(mock_egl, eglSwapBuffers(mock_egl.fake_egl_display, mock_egl.fake_egl_surface))
    .Times(Exactly(1));

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.post_update();
    });
}

TEST_F(AndroidTestFramebufferInit, display_post_successful)
{
    std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        EXPECT_TRUE(buffer.post_update());
    });
}

TEST_F(AndroidTestFramebufferInit, display_post_failure)
{
    using namespace testing;
    std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);

    EXPECT_CALL(mock_egl, eglSwapBuffers(_,_))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_FALSE));

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        EXPECT_FALSE(buffer.post_update());
    });
}

TEST_F(AndroidTestFramebufferInit, framebuffer_correct_view_area)
{
    using namespace testing;
    std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);

    EXPECT_CALL(mock_egl, eglQuerySurface(mock_egl.fake_egl_display, mock_egl.fake_egl_surface,
                                          EGL_WIDTH, _ ))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<3>((EGLint) width),
                        Return(EGL_TRUE)));

    EXPECT_CALL(mock_egl, eglQuerySurface(mock_egl.fake_egl_display, mock_egl.fake_egl_surface,
                                          EGL_HEIGHT, _ ))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<3>((EGLint) height),
                        Return(EGL_TRUE)));

    auto area = display->view_area();

    EXPECT_EQ((int)area.top_left.x.as_uint32_t() , 0);
    EXPECT_EQ((int)area.top_left.y.as_uint32_t() , 0);
    EXPECT_EQ((int)area.size.width.as_uint32_t() , width);
    EXPECT_EQ((int)area.size.height.as_uint32_t(), height);
}
