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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_doubles/mock_android_framebuffer_window.h"
#include "src/server/graphics/android/android_display.h"
#include "mir_test/egl_mock.h"

#include <gtest/gtest.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;

class GPUFramebuffer : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        native_win = std::make_shared<NiceMock<mtd::MockAndroidFramebufferWindow>>();
        mock_egl.silence_uninteresting();
    }

    std::shared_ptr<mtd::MockAndroidFramebufferWindow> native_win;
    mir::EglMock mock_egl;
};

TEST_F(GPUFramebuffer, display_post_calls_swapbuffers_once)
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

TEST_F(GPUFramebuffer, display_post_successful)
{
    std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        EXPECT_TRUE(buffer.post_update());
    });
}

TEST_F(GPUFramebuffer, display_post_failure)
{
    using namespace testing;
    std::shared_ptr<mg::Display> display = std::make_shared<mga::AndroidDisplay>(native_win);

    EXPECT_CALL(this->mock_egl, eglSwapBuffers(_,_))
    .Times(Exactly(1))
    .WillOnce(Return(EGL_FALSE));

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        EXPECT_FALSE(buffer.post_update());
    });
}
