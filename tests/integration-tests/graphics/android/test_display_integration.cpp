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
#include "src/server/graphics/android/hwc11_device.h"
#include "src/server/graphics/android/hwc_display.h"

#include "mir/draw/graphics.h"
#include "mir_test/draw/android_graphics.h"

#include <gtest/gtest.h>
#include <stdexcept>

#include <ui/FramebufferNativeWindow.h>
#include <hardware/hwcomposer.h>

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

        /* note about android_window: OMAP4 drivers seem to only be able to open fb once
           per process (repeated framebuffer_{open,close}() doesn't seem to work). once we
           figure out why, we can remove android_window in the test fixture */
        android_window = std::make_shared< ::android::FramebufferNativeWindow>();

        /* determine hwc11 capable devices so we can skip the hwc11 tests on non supported tests */
        run_hwc11_tests = false;
        const hw_module_t *hw_module;
        int rc = hw_get_module(HWC_HARDWARE_MODULE_ID, &hw_module);
        if (rc == 0) 
        {
            hwc_composer_device_1* hwc_dev;
            rc = hw_module->methods->open(hw_module, HWC_HARDWARE_COMPOSER, (hw_device_t**) &hwc_dev);
            if (rc == 0)
            {
                hwc_device = std::shared_ptr<hwc_composer_device_1>( hwc_dev );
                if (hwc_device->common.version == HWC_DEVICE_API_VERSION_1_1)
                {
                    run_hwc11_tests = true;
                }
            }
        }
    }

    md::glAnimationBasic gl_animation;

    static void TearDownTestCase()
    {
        hwc_device.reset();
        android_window.reset();
    }

    static bool run_hwc11_tests;
    static std::shared_ptr< ::android::FramebufferNativeWindow> android_window;
    static std::shared_ptr<hwc_composer_device_1> hwc_device;
};

std::shared_ptr< ::android::FramebufferNativeWindow> AndroidGPUDisplay::android_window;
std::shared_ptr<hwc_composer_device_1> AndroidGPUDisplay::hwc_device;
bool AndroidGPUDisplay::run_hwc11_tests;

}

/* gpu display tests. These are our back-up display modes, and should be run on every device. */
TEST_F(AndroidGPUDisplay, gpu_display_post_ok)
{
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);
    auto display = std::make_shared<mga::AndroidDisplay>(window);

    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.post_update();
    });
}

TEST_F(AndroidGPUDisplay, gpu_display_ok_with_gles)
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

#define YELLOW "\033[33m"
#define RESET "\033[0m"
#define SUCCEED_IF_NO_HARDWARE_SUPPORT() \
    if(!run_hwc11_tests) { SUCCEED(); std::cout << YELLOW << "--> This device does not have HWC 1.1 support. Skipping test." << RESET << std::endl; return;}

TEST_F(AndroidGPUDisplay, hwc11_ok_with_gles)
{
    SUCCEED_IF_NO_HARDWARE_SUPPORT();

    using namespace testing;

    auto android_window = std::make_shared< ::android::FramebufferNativeWindow>();
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);
    auto hwc = std::make_shared<mga::HWC11Device>(hwc_device);
    auto display = std::make_shared<mga::HWCDisplay>(window, hwc);

    gl_animation.init_gl();

    display->for_each_display_buffer([this](mg::DisplayBuffer& buffer)
    {
        gl_animation.render_gl();
        buffer.post_update();

        gl_animation.render_gl();
        buffer.post_update();
    });
}
