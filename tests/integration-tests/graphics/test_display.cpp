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
#include "mir/graphics/android/android_display.h" 
#include <ui/FramebufferNativeWindow.h>


#include <GLES2/gl2.h>
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;

class AndroidFramebufferIntegration : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;
        android_window = std::shared_ptr<ANativeWindow> ((ANativeWindow*) new android::FramebufferNativeWindow);
        window = std::shared_ptr<mga::AndroidFramebufferWindow> (new mga::AndroidFramebufferWindow(android_window));
    }
    std::shared_ptr<ANativeWindow> android_window;
    std::shared_ptr<mga::AndroidFramebufferWindow> window;
};


TEST_F(AndroidFramebufferIntegration, init_does_not_throw)
{
    using namespace testing;
    std::shared_ptr<mga::AndroidDisplay> display;

    EXPECT_NO_THROW({
    display = std::shared_ptr<mga::AndroidDisplay>(new mga::AndroidDisplay(window));
    display->post_update();
    });

}
