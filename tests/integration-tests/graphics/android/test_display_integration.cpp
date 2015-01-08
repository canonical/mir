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
#include "mir/glib_main_loop.h"

#include "examples/graphics.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/stub_gl_config.h"
#include "mir_test_doubles/stub_renderable.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/advanceable_clock.h"

#include <gtest/gtest.h>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;
namespace md=mir::draw;
namespace mt=mir::test;
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
    struct StubHwcConfig : public mga::HwcConfiguration
    {
        void power_mode(mga::DisplayName, MirPowerMode) override {}
        mga::DisplayAttribs active_attribs_for(mga::DisplayName) override
        {
            return mga::DisplayAttribs{{0,0}, {0,0}, 0.0, true};
        } 
        mga::ConfigChangeSubscription subscribe_to_config_changes(
            std::function<void()> const& cb) override
        {
            hotplug_fn = cb;
            return {};
        }
        void simulate_hotplug()
        {
            hotplug_fn();
        }
        std::function<void()> hotplug_fn{[]{}};
    };

    struct WrappingConfig : public mga::HwcConfiguration
    {
        WrappingConfig(mga::HwcConfiguration& config) : wrapped(config) {}

        void power_mode(mga::DisplayName d, MirPowerMode m) override
        {
            wrapped.power_mode(d, m);
        }
        mga::DisplayAttribs active_attribs_for(mga::DisplayName d) override
        {
            return wrapped.active_attribs_for(d);
        } 
        mga::ConfigChangeSubscription subscribe_to_config_changes(
            std::function<void()> const& cb) override
        {
            return wrapped.subscribe_to_config_changes(cb);
        }
        mga::HwcConfiguration& wrapped;
    };

    struct StubOutputBuilder : public mga::DisplayBufferBuilder
    {
        MirPixelFormat display_format() override
        {
            return mir_pixel_format_abgr_8888;
        }
        std::unique_ptr<mga::ConfigurableDisplayBuffer> create_display_buffer(
            mg::GLProgramFactory const&, mga::GLContext const&) override
        {
            struct NullConfigurableDisplayBuffer : mga::ConfigurableDisplayBuffer
            {
                geom::Rectangle view_area() const override { return geom::Rectangle(); }
                void make_current() override {}
                void release_current() override {}
                void gl_swap_buffers() override {}
                void flip() override {}
                bool post_renderables_if_optimizable(mg::RenderableList const&) override { return false; }
                MirOrientation orientation() const override { return mir_orientation_normal; }
                bool uses_alpha() const override { return false; }
                void configure(MirPowerMode, MirOrientation) {}
            };
            return std::unique_ptr<mga::ConfigurableDisplayBuffer>(new NullConfigurableDisplayBuffer());
        }
        std::unique_ptr<mga::HwcConfiguration> create_hwc_configuration() override
        {
            return std::unique_ptr<mga::HwcConfiguration>(new WrappingConfig(stub_config));
        }
        StubHwcConfig stub_config;
    };

    AndroidDisplayHotplug() :
        loop([this]{ mainloop.run(); })
    {
    }

    ~AndroidDisplayHotplug()
    {
        mainloop.stop();
        if (loop.joinable())
            loop.join();
    }

    std::thread loop;
    std::shared_ptr<mtd::AdvanceableClock> clock{std::make_shared<mtd::AdvanceableClock>()};
    mir::GLibMainLoop mainloop{clock};
    std::shared_ptr<StubOutputBuilder> stub_output_builder{std::make_shared<StubOutputBuilder>()};
    mga::Display display{
        stub_output_builder,
        std::make_shared<mg::ProgramFactory>(),
        std::make_shared<mtd::StubGLConfig>(),
        mir::report::null_display_report()};
};

TEST_F(AndroidDisplayHotplug, hotplug_generates_mainloop_event)
{
    std::atomic<int> call_count{0};
    std::function<void()> change_handler = [&call_count]
    {
        call_count++;
    };

    display.register_configuration_change_handler(mainloop, change_handler);
    EXPECT_THAT(call_count, testing::Eq(0));

    stub_output_builder->stub_config.simulate_hotplug();
    EXPECT_THAT(call_count, testing::Eq(1));
    stub_output_builder->stub_config.simulate_hotplug();
    EXPECT_THAT(call_count, testing::Eq(2));
}
