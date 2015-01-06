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

#include "mir/graphics/display_buffer.h"
#include "src/platforms/android/display.h"
#include "src/platforms/android/hwc_loggers.h"
#include "src/platforms/android/resource_factory.h"
#include "src/platforms/android/android_graphic_buffer_allocator.h"
#include "src/platforms/android/output_builder.h"
#include "src/server/graphics/program_factory.h"
#include "src/server/report/null_report_factory.h"
//#include "mir/glib_main_loop.h"

#include "examples/graphics.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/stub_gl_config.h"
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test/fake_clock.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;
namespace md=mir::draw;
namespace mtd=mir::test::doubles;

namespace
{
void (*original_sigterm_handler)(int);
std::shared_ptr<mg::Display> display;
std::shared_ptr<mga::GraphicBufferAllocator> buffer_allocator;

class AndroidDisplay : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        /* note: exynos5 hwc driver can sends sigterm to vsync thread when closing hwc.
           the server can handle this, but we need the test to as well */
        original_sigterm_handler = signal(SIGTERM, [](int){});

        buffer_allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>();

        /* note about fb_device: OMAP4 drivers seem to only be able to open fb once
           per process (repeated framebuffer_{open,close}() doesn't seem to work). once we
           figure out why, we can remove fb_device in the test fixture */
        auto report = std::make_shared<mga::NullHwcReport>();
        auto display_resource_factory = std::make_shared<mga::ResourceFactory>();
        auto null_display_report = mir::report::null_display_report();
        auto stub_gl_config = std::make_shared<mtd::StubGLConfig>();
        auto display_buffer_factory = std::make_shared<mga::OutputBuilder>(
            buffer_allocator, display_resource_factory, mga::OverlayOptimization::enabled, report);
        auto program_factory = std::make_shared<mg::ProgramFactory>();
        display = std::make_shared<mga::Display>(
            display_buffer_factory, program_factory, stub_gl_config, null_display_report);
    }

    static void TearDownTestCase()
    {
        signal(SIGTERM, original_sigterm_handler);
        display.reset();
        buffer_allocator.reset();
    }
};
}

TEST_F(AndroidDisplay, display_can_post)
{
    display->for_each_display_buffer([](mg::DisplayBuffer& buffer)
    {
        buffer.make_current();
        md::glAnimationBasic gl_animation;
        gl_animation.init_gl();

        gl_animation.render_gl();
        buffer.gl_swap_buffers();
        buffer.flip();

        gl_animation.render_gl();
        buffer.gl_swap_buffers();
        buffer.flip();
    });
}

TEST_F(AndroidDisplay, display_can_post_overlay)
{
    display->for_each_display_buffer([](mg::DisplayBuffer& db)
    {
        db.make_current();
        auto area = db.view_area();
        auto buffer = buffer_allocator->alloc_buffer_platform(
            area.size, mir_pixel_format_abgr_8888, mga::BufferUsage::use_hardware);
        mg::RenderableList list{
            std::make_shared<mtd::StubRenderable>(buffer, area)
        };

        db.post_renderables_if_optimizable(list);
    });
}

//doesn't use the HW modules, rather tests mainloop integration
struct AndroidDisplayHotplug : ::testing::Test
{
    struct StubOutputBuilder : public mga::DisplayBufferBuilder
    {
        MirPixelFormat display_format() override
        {
            return mir_pixel_format_abgr_8888;
        }

        std::unique_ptr<mga::ConfigurableDisplayBuffer> create_display_buffer(
            mg::GLProgramFactory const&, mga::GLContext const&) override
        {
            return nullptr;
        }

        std::unique_ptr<mga::HwcConfiguration> create_hwc_configuration() override
        {
            return nullptr;
        }
    };

#if 0
    mir::GLibMainLoop mainloop(std::make_shared<mt::FakeClock>());
    auto buffer_allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>();
    auto display_buffer_factory = std::make_shared<mga::OutputBuilder>(
        buffer_allocator, stub_resource_factory, mga::OverlayOptimization::enabled, report);
    auto stub_gl_config = std::make_shared<mtd::StubGLConfig>();
    auto program_factory = std::make_shared<mg::ProgramFactory>();
    auto null_display_report = mir::report::null_display_report();
    auto display = std::make_shared<mga::Display>(
        display_buffer_factory, program_factory, stub_gl_config, null_display_report);
#endif
};

TEST_F(AndroidDisplay, hotplug_generates_mainloop_event)
{
#if 0
    std::atomic<int> call_count = 0;
    std::function<void()> change_handler = [&call_count]
    {
        call_count++;
    };

    display->register_configuration_change_handler(mainloop, change_handler);

    stub_resource_factory->simulate_hotplug();
    stub_resource_factory->simulate_hotplug();
    EXPECT_THAT(call_count, testing::Eq(2));
#endif
}
