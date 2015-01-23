/*
 * Copyright © 2015 Canonical Ltd.
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

#include "src/platforms/android/server/display.h"
#include "src/platforms/android/server/hal_component_factory.h"
#include "src/server/graphics/program_factory.h"
#include "src/server/report/null_report_factory.h"
#include "mir/glib_main_loop.h"
#include "mir/time/steady_clock.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_framebuffer_bundle.h"
#include "mir_test_doubles/stub_gl_config.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/mock_gl.h"
#include "mir_test/auto_unblock_thread.h"
#include <gtest/gtest.h>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;

//doesn't use the HW modules, rather tests mainloop integration
struct DisplayHotplug : ::testing::Test
{
    struct StubHwcConfig : public mga::HwcConfiguration
    {
        void power_mode(mga::DisplayName, MirPowerMode) override {}
        mga::DisplayAttribs active_attribs_for(mga::DisplayName) override
        {
            return mga::DisplayAttribs{{0,0}, {0,0}, 0.0, true, mir_pixel_format_abgr_8888, 2};
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

    struct StubOutputBuilder : public mga::DisplayComponentFactory
    {
        std::unique_ptr<mga::FramebufferBundle> create_framebuffers(mga::DisplayAttribs const&) override
        {
            return std::unique_ptr<mga::FramebufferBundle>(new testing::NiceMock<mtd::MockFBBundle>());
        }

        std::unique_ptr<mga::DisplayDevice> create_display_device() override
        {
            return std::unique_ptr<mga::DisplayDevice>(new testing::NiceMock<mtd::MockDisplayDevice>());
        }

        std::unique_ptr<mga::HwcConfiguration> create_hwc_configuration() override
        {
            return std::unique_ptr<mga::HwcConfiguration>(new WrappingConfig(stub_config));
        }
        StubHwcConfig stub_config;
    };

    testing::NiceMock<mtd::MockEGL> mock_egl;
    testing::NiceMock<mtd::MockGL> mock_gl;
    mir::GLibMainLoop mainloop{std::make_shared<mir::time::SteadyClock>()};
    mir::test::AutoUnblockThread loop{
            [this]{ mainloop.enqueue(this, [this] { mainloop.stop(); }); },
            [this]{ mainloop.run(); }};
    std::shared_ptr<StubOutputBuilder> stub_output_builder{std::make_shared<StubOutputBuilder>()};
    mga::Display display{
        stub_output_builder,
        std::make_shared<mg::ProgramFactory>(),
        std::make_shared<mtd::StubGLConfig>(),
        mir::report::null_display_report(),
        mga::OverlayOptimization::enabled};
};

TEST_F(DisplayHotplug, hotplug_generates_mainloop_event)
{
    std::mutex mutex;
    std::condition_variable cv;
    int call_count{0};
    std::function<void()> change_handler{[&]
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        call_count++;
        cv.notify_all();
    }};
    std::unique_lock<decltype(mutex)> lk(mutex);
    display.register_configuration_change_handler(mainloop, change_handler);
    stub_output_builder->stub_config.simulate_hotplug();
    EXPECT_TRUE(cv.wait_for(lk, std::chrono::seconds(2), [&]{ return call_count == 1; }));

    stub_output_builder->stub_config.simulate_hotplug();
    EXPECT_TRUE(cv.wait_for(lk, std::chrono::seconds(2), [&]{ return call_count == 2; }));
    mainloop.unregister_fd_handler(&display);
}
