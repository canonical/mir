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

#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/display_buffer.h"
#include "src/server/graphics/android/android_display.h"
#include "src/server/graphics/android/hwc10_device.h"
#include "src/server/graphics/android/hwc11_device.h"
#include "src/server/graphics/android/hwc_layerlist.h"
#include "src/server/graphics/android/hwc_vsync.h"
#include "src/server/graphics/android/default_framebuffer_factory.h"
#include "src/server/graphics/android/android_graphic_buffer_allocator.h"
#include "src/server/graphics/android/gpu_android_display_buffer_factory.h"
#include "src/server/graphics/android/hwc_android_display_buffer_factory.h"
#include "src/server/graphics/android/fb_device.h"

#include "examples/graphics.h"
#include "mir_test/draw/android_graphics.h"
#include "mir_test_doubles/mock_display_report.h"

#include <gtest/gtest.h>
#include <stdexcept>

#include <hardware/hwcomposer.h>

namespace mc=mir::compositor;
namespace geom=mir::geometry;
namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace md=mir::draw;
namespace mtd=mir::test::doubles;

namespace
{
class AndroidGPUDisplay : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        /* this is an important precondition for acquiring the display! */
        ASSERT_FALSE(mir::test::draw::is_surface_flinger_running());

        /* note about fb_device: OMAP4 drivers seem to only be able to open fb once
           per process (repeated framebuffer_{open,close}() doesn't seem to work). once we
           figure out why, we can remove fb_device in the test fixture */
        auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
        auto allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer); 
        fb_factory = std::make_shared<mga::DefaultFramebufferFactory>(allocator);
        fb_device = fb_factory->create_fb_device();

        /* determine hwc11 capable devices so we can skip the hwc11 tests on non supported tests */
        hw_module_t const* gr_module;
        hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &gr_module);
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
                if (hwc_device->common.version == HWC_DEVICE_API_VERSION_1_0)
                {
                    run_hwc10_tests = true;
                }
            }
        }
    }

    md::glAnimationBasic gl_animation;

    static bool run_hwc11_tests;
    static bool run_hwc10_tests;
    static std::shared_ptr<mga::FramebufferFactory> fb_factory;
    static std::shared_ptr<mga::DisplaySupportProvider> fb_device;
    static std::shared_ptr<hwc_composer_device_1> hwc_device;
};

std::shared_ptr<mga::DisplaySupportProvider> AndroidGPUDisplay::fb_device;
std::shared_ptr<mga::FramebufferFactory> AndroidGPUDisplay::fb_factory;
std::shared_ptr<hwc_composer_device_1> AndroidGPUDisplay::hwc_device;
bool AndroidGPUDisplay::run_hwc11_tests;
bool AndroidGPUDisplay::run_hwc10_tests;
}

/* gpu display tests. These are our back-up display modes, and should be run on every device. */
TEST_F(AndroidGPUDisplay, gpu_display_ok_with_gles)
{
    auto mock_display_report = std::make_shared<testing::NiceMock<mtd::MockDisplayReport>>();
    auto fb_window = fb_factory->create_fb_native_window(fb_device);
    auto window_query = std::make_shared<mga::AndroidFramebufferWindow>(fb_window);
    auto db_factory = std::make_shared<mga::GPUAndroidDisplayBufferFactory>();
    auto display = std::make_shared<mga::AndroidDisplay>(window_query, db_factory, fb_device, mock_display_report);

    display->for_each_display_buffer([this](mg::DisplayBuffer& buffer)
    {
        buffer.make_current();
        gl_animation.init_gl();

        gl_animation.render_gl();
        buffer.post_update();

        gl_animation.render_gl();
        buffer.post_update();
    });
}

#define YELLOW "\033[33m"
#define RESET "\033[0m"
#define SUCCEED_IF_NO_HWC11_SUPPORT() \
    if(!run_hwc11_tests) { SUCCEED(); std::cout << YELLOW << "--> This device does not have HWC 1.1 support. Skipping test." << RESET << std::endl; return;}
#define SUCCEED_IF_NO_HWC10_SUPPORT() \
    if(!run_hwc10_tests) { SUCCEED(); std::cout << YELLOW << "--> This device does not have HWC 1.0 support. Skipping test." << RESET << std::endl; return;}

TEST_F(AndroidGPUDisplay, hwc10_ok_with_gles)
{
    SUCCEED_IF_NO_HWC10_SUPPORT();

    auto mock_display_report = std::make_shared<testing::NiceMock<mtd::MockDisplayReport>>();
    auto fb_window = fb_factory->create_fb_native_window(fb_device);
    auto window_query = std::make_shared<mga::AndroidFramebufferWindow>(fb_window);
    auto layerlist = std::make_shared<mga::HWCLayerList>();

    auto syncer = std::make_shared<mga::HWCVsync>();
    auto hwc = std::make_shared<mga::HWC10Device>(hwc_device, fb_device, syncer);
    auto db_factory = std::make_shared<mga::HWCAndroidDisplayBufferFactory>(hwc);
    auto display = std::make_shared<mga::AndroidDisplay>(window_query, db_factory, hwc,
                                                         mock_display_report);

    display->for_each_display_buffer([this](mg::DisplayBuffer& buffer)
    {
        buffer.make_current();
        gl_animation.init_gl();

        gl_animation.render_gl();
        buffer.post_update();

        gl_animation.render_gl();
        buffer.post_update();
    });
}

TEST_F(AndroidGPUDisplay, hwc11_ok_with_gles)
{
    SUCCEED_IF_NO_HWC11_SUPPORT();

    auto mock_display_report = std::make_shared<testing::NiceMock<mtd::MockDisplayReport>>();
    auto fb_window = fb_factory->create_fb_native_window(fb_device);
    auto window_query = std::make_shared<mga::AndroidFramebufferWindow>(fb_window);
    auto layerlist = std::make_shared<mga::HWCLayerList>();

    auto syncer = std::make_shared<mga::HWCVsync>();
    auto hwc = std::make_shared<mga::HWC11Device>(hwc_device, layerlist, fb_device, syncer);
    auto db_factory = std::make_shared<mga::HWCAndroidDisplayBufferFactory>(hwc);
    auto display = std::make_shared<mga::AndroidDisplay>(window_query, db_factory, hwc, mock_display_report);

    display->for_each_display_buffer([this](mg::DisplayBuffer& buffer)
    {
        buffer.make_current();
        gl_animation.init_gl();

        gl_animation.render_gl();
        buffer.post_update();

        gl_animation.render_gl();
        buffer.post_update();
    });
}
