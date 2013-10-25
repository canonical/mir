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
#include "src/server/graphics/android/resource_factory.h"
#include "src/server/graphics/android/android_graphic_buffer_allocator.h"
#include "src/server/graphics/android/display_buffer_factory.h"
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
        /* note about fb_device: OMAP4 drivers seem to only be able to open fb once
           per process (repeated framebuffer_{open,close}() doesn't seem to work). once we
           figure out why, we can remove fb_device in the test fixture */
        auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
        buffer_allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer); 
        resource_factory = std::make_shared<mga::ResourceFactory>();
        fb_device = resource_factory->create_fb_native_device();
        hwc_device = resource_factory->create_hwc_native_device();

        /* determine hwc11 capable devices so we can skip the hwc11 tests on non supported tests */
        if (hwc_device->common.version == HWC_DEVICE_API_VERSION_1_1)
        {
            run_hwc11_tests = true;
        }
        if (hwc_device->common.version == HWC_DEVICE_API_VERSION_1_0)
        {
            run_hwc10_tests = true;
        }
    }

    md::glAnimationBasic gl_animation;

    static bool run_hwc11_tests;
    static bool run_hwc10_tests;
    static std::shared_ptr<mga::AndroidGraphicBufferAllocator> buffer_allocator;
    static std::shared_ptr<mga::ResourceFactory> resource_factory;
    static std::shared_ptr<framebuffer_device_t> fb_device;
    static std::shared_ptr<hwc_composer_device_1> hwc_device;
};

std::shared_ptr<mga::AndroidGraphicBufferAllocator> AndroidGPUDisplay::buffer_allocator;
std::shared_ptr<mga::ResourceFactory> AndroidGPUDisplay::resource_factory;
std::shared_ptr<framebuffer_device_t> AndroidGPUDisplay::fb_device;
std::shared_ptr<hwc_composer_device_1> AndroidGPUDisplay::hwc_device;
bool AndroidGPUDisplay::run_hwc11_tests;
bool AndroidGPUDisplay::run_hwc10_tests;
}

/* gpu display tests. These are our back-up display modes, and should be run on every device. */
TEST_F(AndroidGPUDisplay, gpu_display_ok_with_gles)
{
    auto mock_display_report = std::make_shared<testing::NiceMock<mtd::MockDisplayReport>>();
    auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto device = resource_factory->create_fb_device(fb_device);
    auto fb_swapper = resource_factory->create_fb_buffers(device, buffer_allocator);
    auto display = resource_factory->create_display(fb_swapper, device, mock_display_report);

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
    auto device = resource_factory->create_hwc10_device(hwc_device, fb_device);
    auto fb_swapper = resource_factory->create_fb_buffers(device, buffer_allocator);
    auto display = resource_factory->create_display(fb_swapper, device, mock_display_report);

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
    auto device = resource_factory->create_hwc11_device(hwc_device);
    auto fb_swapper = resource_factory->create_fb_buffers(device, buffer_allocator);
    auto display = resource_factory->create_display(fb_swapper, device, mock_display_report);

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
