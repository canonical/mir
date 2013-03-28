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

#include "src/server/graphics/android/android_platform.h"
#include "src/server/graphics/android/android_buffer_allocator.h"
#include "src/server/graphics/android/android_display.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/null_display_report.h"
#include "mir/compositor/swapper_factory.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_bundle_surfaces.h"
#include "mir/compositor/buffer_properties.h"

#include "mir/draw/graphics.h"
#include "mir_test/draw/android_graphics.h"
#include "mir_test/draw/patterns.h"

#include <gtest/gtest.h>
#include <stdexcept>

#include <ui/FramebufferNativeWindow.h>
namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace md=mir::draw;
namespace mtd=mir::test::draw;

namespace
{

class AndroidGPUDisplay : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        /* this is an important precondition for acquiring the display! */
        ASSERT_FALSE(mtd::is_surface_flinger_running());

        /* note about android_window: android drivers seem to only be able to open fb once
           per process (gralloc's framebuffer_close() doesn't seem to work). once we
           figure out why, we can put display in the test fixture */
        android_window = std::make_shared< ::android::FramebufferNativeWindow>();
    }

    virtual void SetUp()
    {
    }

    md::glAnimationBasic gl_animation;

    static std::shared_ptr< ::android::FramebufferNativeWindow> android_window;
};

std::shared_ptr< ::android::FramebufferNativeWindow> AndroidGPUDisplay::android_window;

}

TEST_F(AndroidGPUDisplay, display_creation_ok)
{
    EXPECT_NO_THROW({
        auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);
        auto display = std::make_shared<mga::AndroidDisplay>(window);
    });
}

TEST_F(AndroidGPUDisplay, display_post_ok)
{
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);
    auto display = std::make_shared<mga::AndroidDisplay>(window);

    EXPECT_NO_THROW(
    {
        display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
        {
            buffer.post_update();
        });
    });
}

TEST_F(AndroidGPUDisplay, display_clear_and_post_ok)
{
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);
    auto display = std::make_shared<mga::AndroidDisplay>(window);

    EXPECT_NO_THROW(
    {
        display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
        {
            glClearColor(0.0f,0.0f,0.0f,1.0f);
            glClear(GL_COLOR_BUFFER_BIT); 
            buffer.post_update();
        });
    });
}

TEST_F(AndroidGPUDisplay, buffer_ok_with_gles_context)
{
    using namespace testing;

    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);
    auto display = std::make_shared<mga::AndroidDisplay>(window);

    gl_animation.init_gl();

    display->for_each_display_buffer([this](mg::DisplayBuffer& buffer)
    {
        gl_animation.render_gl();
        buffer.post_update();

        gl_animation.render_gl();
        buffer.post_update();
    });
}
