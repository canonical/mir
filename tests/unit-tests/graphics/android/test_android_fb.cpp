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
#include "src/platform/graphics/android/android_display.h"
#include "mir_test_doubles/mock_display_report.h"
#include "mir_test_doubles/mock_display_device.h"
#include "mir_test_doubles/mock_egl.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_display_builder.h"
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

        dummy_display = mock_egl.fake_egl_display;
        dummy_context = mock_egl.fake_egl_context;
        dummy_config = mock_egl.fake_configs[0];

        mock_display_report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();
        stub_db_factory = std::make_shared<mtd::StubDisplayBuilder>(display_size);
    }

    EGLConfig dummy_config;
    EGLDisplay dummy_display;
    EGLContext dummy_context;

    std::shared_ptr<mtd::MockDisplayReport> mock_display_report;
    std::shared_ptr<mtd::StubDisplayBuilder> stub_db_factory;
    testing::NiceMock<mtd::MockEGL> mock_egl;
};

TEST_F(AndroidDisplayTest, display_creation)
{
    using namespace testing;
    EGLSurface fake_surface = (EGLSurface) 0x715;
    EGLint const expected_pbuffer_attr[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLint const expected_context_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    //on constrution
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .Times(1)
        .WillOnce(Return(dummy_display));
    EXPECT_CALL(mock_egl, eglInitialize(dummy_display, _, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<1>(1), SetArgPointee<2>(4), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglCreateContext(
        dummy_display, _, EGL_NO_CONTEXT, mtd::AttrMatches(expected_context_attr)))
        .Times(1)
        .WillOnce(Return(dummy_context));
    EXPECT_CALL(mock_egl, eglCreatePbufferSurface(
        dummy_display, _, mtd::AttrMatches(expected_pbuffer_attr)))
        .Times(1)
        .WillOnce(Return(fake_surface));
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, fake_surface, fake_surface, dummy_context))
        .Times(1);

    //on destruction
    EXPECT_CALL(mock_egl, eglGetCurrentContext())
        .Times(1)
        .WillOnce(Return(dummy_context));
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT))
        .Times(1);
    EXPECT_CALL(mock_egl, eglDestroySurface(dummy_display, fake_surface))
        .Times(1);
    EXPECT_CALL(mock_egl, eglDestroyContext(dummy_display, dummy_context))
        .Times(1);
    EXPECT_CALL(mock_egl, eglTerminate(dummy_display))
        .Times(1);

    mga::AndroidDisplay display(stub_db_factory, mock_display_report);
}

TEST_F(AndroidDisplayTest, display_egl_config_selection)
{
    using namespace testing;
    int const incorrect_visual_id = 2;
    int const correct_visual_id = 1;
    EGLint const num_cfgs = 45;
    EGLint const expected_cfg_attr [] =
        { EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE };
    EGLConfig selected_config;
    EGLConfig cfgs[45];
    for(auto i = 0; i < num_cfgs; i++)
        cfgs[i] = reinterpret_cast<EGLConfig>(i);
    int config_to_select_index = 37;
    EGLConfig correct_config = cfgs[config_to_select_index];

    ON_CALL(mock_egl, eglGetConfigAttrib(_, _, EGL_NATIVE_VISUAL_ID, _))
        .WillByDefault(DoAll(SetArgPointee<3>(incorrect_visual_id), Return(EGL_TRUE)));
    ON_CALL(mock_egl, eglGetConfigAttrib(dummy_display, correct_config, EGL_NATIVE_VISUAL_ID,_))
        .WillByDefault(DoAll(SetArgPointee<3>(correct_visual_id), Return(EGL_TRUE)));
    ON_CALL(mock_egl, eglCreateContext(_,_,_,_))
        .WillByDefault(DoAll(SaveArg<1>(&selected_config),Return(dummy_context)));

    auto config_filler = [&]
    (EGLDisplay, EGLint const*, EGLConfig* out_cfgs, EGLint, EGLint* out_num_cfgs) -> EGLBoolean
    {
        memcpy(out_cfgs, cfgs, sizeof(EGLConfig) * num_cfgs);
        *out_num_cfgs = num_cfgs;
        return EGL_TRUE;
    };

    EXPECT_CALL(mock_egl, eglGetConfigs(dummy_display, NULL, 0, _))
        .Times(1)
        .WillOnce(DoAll(SetArgPointee<3>(num_cfgs), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglChooseConfig(dummy_display, mtd::AttrMatches(expected_cfg_attr),_,num_cfgs,_))
        .Times(1)
        .WillOnce(Invoke(config_filler));

    mga::AndroidDisplay display(stub_db_factory, mock_display_report);
    EXPECT_EQ(correct_config, selected_config);
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

TEST_F(AndroidDisplayTest, test_dpms_configuration_changes_reach_device)
{
    using namespace testing;
    auto mock_display_device = std::make_shared<NiceMock<mtd::MockDisplayDevice>>();
    Sequence seq;
    EXPECT_CALL(*mock_display_device, mode(mir_power_mode_on))
        .InSequence(seq);
    EXPECT_CALL(*mock_display_device, mode(mir_power_mode_standby))
        .InSequence(seq);
    EXPECT_CALL(*mock_display_device, mode(mir_power_mode_off))
        .InSequence(seq);
    EXPECT_CALL(*mock_display_device, mode(mir_power_mode_suspend))
        .InSequence(seq);

    auto stub_db_factory = std::make_shared<mtd::StubDisplayBuilder>(mock_display_device);
    mga::AndroidDisplay display(stub_db_factory, mock_display_report);

    auto configuration = display.configuration();
    configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output)
    {
        configuration->configure_output(
            output.id, output.used, output.top_left, output.current_mode_index, mir_power_mode_on);
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output)
    {
        configuration->configure_output(
            output.id, output.used, output.top_left, output.current_mode_index, mir_power_mode_standby);
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output)
    {
        configuration->configure_output(
            output.id, output.used, output.top_left, output.current_mode_index, mir_power_mode_off);
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::DisplayConfigurationOutput const& output)
    {
        configuration->configure_output(
            output.id, output.used, output.top_left, output.current_mode_index, mir_power_mode_suspend);
    });
    display.configure(*configuration);
}
