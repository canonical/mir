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
#include "mir/graphics/display_configuration.h"
#include "mir/logging/logger.h"
#include "src/server/graphics/android/android_display.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/mock_display_buffer_factory.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir_test_doubles/stub_driver_interpreter.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace ml=mir::logging;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

struct DummyGPUDisplayType {};
struct DummyHWCDisplayType {};

static geom::Size const display_size{433,232};

class AndroidDisplayTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        using namespace testing;

        visual_id = 5;
        dummy_display = reinterpret_cast<EGLDisplay>(0x34);
        dummy_config = reinterpret_cast<EGLConfig>(0x44);
        dummy_context = reinterpret_cast<EGLContext>(0x58);

        mock_display_report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();
        auto stub_driver_interpreter = std::make_shared<mtd::StubDriverInterpreter>(display_size, visual_id);
        native_win = std::make_shared<mg::android::MirNativeWindow>(stub_driver_interpreter);
        stub_db_factory = std::make_shared<mtd::MockDisplayBufferFactory>(display_size, dummy_display, dummy_config, dummy_context);
    }

    int visual_id;
    EGLConfig dummy_config;
    EGLDisplay dummy_display;
    EGLContext dummy_context;

    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
    std::shared_ptr<ANativeWindow> native_win;
    std::shared_ptr<mtd::MockDisplayBufferFactory> stub_db_factory;
    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(AndroidDisplayTest, display_creation)
{
    using namespace testing;
    EGLint const expected_pbuffer_attr[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };

    EGLSurface fake_surface = (EGLSurface) 0x715;
    EXPECT_CALL(mock_egl, eglCreatePbufferSurface(
        dummy_display, dummy_config, mtd::AttrMatches(expected_pbuffer_attr)))
        .Times(1)
        .WillOnce(Return(fake_surface));

    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, fake_surface, fake_surface, dummy_context))
        .Times(1);

    //on destruction
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT))
        .Times(1);

    mga::AndroidDisplay display(stub_db_factory, mock_display_report);
}

TEST_F(AndroidDisplayTest, display_logging)
{
    using namespace testing;
    EXPECT_CALL(*mock_display_report, report_successful_setup_of_native_resources())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_egl_configuration(_,_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_successful_egl_make_current_on_construction())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_successful_display_construction())
        .Times(1);

    EXPECT_CALL(mock_egl, eglMakeCurrent(_,_,_,_))
        .Times(AtLeast(1));

    mga::AndroidDisplay display(stub_db_factory, mock_display_report);
}

TEST_F(AndroidDisplayTest, eglMakeCurrent_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_display_report, report_successful_setup_of_native_resources())
        .Times(1);
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, _, _, _))
        .Times(1)
        .WillOnce(Return(EGL_FALSE));
    EXPECT_CALL(*mock_display_report, report_successful_egl_make_current_on_construction())
        .Times(0);
    EXPECT_CALL(*mock_display_report, report_successful_display_construction())
        .Times(0);

    EXPECT_THROW({
        mga::AndroidDisplay display(stub_db_factory, mock_display_report);
    }, std::runtime_error);
}

TEST_F(AndroidDisplayTest, startup_logging_error_because_of_surface_creation_failure)
{
    using namespace testing;

    EXPECT_CALL(*mock_display_report, report_successful_setup_of_native_resources())
        .Times(0);
    EXPECT_CALL(*mock_display_report, report_successful_egl_make_current_on_construction())
        .Times(0);
    EXPECT_CALL(*mock_display_report, report_successful_display_construction())
        .Times(0);

    EXPECT_CALL(mock_egl, eglCreatePbufferSurface(_,_,_))
        .Times(1)
        .WillOnce(Return(EGL_NO_SURFACE));

    EXPECT_THROW({
        mga::AndroidDisplay display(stub_db_factory, mock_display_report);
    }, std::runtime_error);
}

//we only have single display and single mode on android for the time being
TEST_F(AndroidDisplayTest, android_display_configuration_info)
{
    mga::AndroidDisplay display(stub_db_factory, mock_display_report);
    auto config = display.configuration();

    std::vector<mg::DisplayConfigurationOutput> configurations;
    config->for_each_output([&](mg::DisplayConfigurationOutput const& config)
    {
        configurations.push_back(config);
    });

    ASSERT_EQ(1u, configurations.size());
    auto& disp_conf = configurations[0];
    ASSERT_EQ(1u, disp_conf.modes.size());
    auto& disp_mode = disp_conf.modes[0];
    EXPECT_EQ(display_size, disp_mode.size);

    EXPECT_EQ(mg::DisplayConfigurationOutputId{1}, disp_conf.id); 
    EXPECT_EQ(mg::DisplayConfigurationCardId{0}, disp_conf.card_id); 
    EXPECT_TRUE(disp_conf.connected);
    EXPECT_TRUE(disp_conf.used);
    auto origin = geom::Point{0,0}; 
    EXPECT_EQ(origin, disp_conf.top_left);
    EXPECT_EQ(0, disp_conf.current_mode_index);

    //TODO fill refresh rate accordingly
    //TODO fill physical_size_mm fields accordingly;
}

#if 0
//TODO: this test ties the display buffer to the display.
TEST_F(AndroidDisplayBufferTest, test_dpms_configuration_changes_reach_device)
{
    using namespace testing;

    geom::Size fake_display_size{223, 332};
    EXPECT_CALL(*mock_display_info, display_size())
        .Times(1)
        .WillOnce(Return(fake_display_size)); 
    auto display = create_display();
    
    auto on_configuration = display->configuration();
    on_configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        on_configuration->configure_output(output.id, output.used, output.top_left, output.current_mode_index,
                                           mir_power_mode_on);
    });
    auto off_configuration = display->configuration();
    off_configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        off_configuration->configure_output(output.id, output.used, output.top_left, output.current_mode_index,
                                           mir_power_mode_off);
    });
    auto standby_configuration = display->configuration();
    standby_configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        standby_configuration->configure_output(output.id, output.used, output.top_left, output.current_mode_index,
                                           mir_power_mode_standby);
    });
    auto suspend_configuration = display->configuration();
    suspend_configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output) -> void
    {
        suspend_configuration->configure_output(output.id, output.used, output.top_left, output.current_mode_index,
                                           mir_power_mode_suspend);
    });

    {
        InSequence seq;
        EXPECT_CALL(*mock_display_commander, mode(mir_power_mode_on));
        EXPECT_CALL(*mock_display_commander, mode(mir_power_mode_off));
        EXPECT_CALL(*mock_display_commander, mode(mir_power_mode_suspend));
        EXPECT_CALL(*mock_display_commander, mode(mir_power_mode_standby));
    }
    display->configure(*on_configuration);
    display->configure(*off_configuration);
    display->configure(*suspend_configuration);
    display->configure(*standby_configuration);
}
#endif
