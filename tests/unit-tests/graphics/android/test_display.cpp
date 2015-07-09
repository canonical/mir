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
#include "src/platforms/android/server/display.h"
#include "src/server/report/null_report_factory.h"
#include "mir/test/doubles/mock_display_report.h"
#include "mir/test/doubles/mock_display_device.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_display_buffer.h"
#include "mir/test/doubles/stub_display_builder.h"
#include "mir/test/doubles/stub_gl_config.h"
#include "mir/test/doubles/mock_gl_config.h"
#include "mir/test/doubles/stub_gl_program_factory.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir/test/doubles/stub_driver_interpreter.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <algorithm>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mtd=mir::test::doubles;
namespace geom=mir::geometry;

class Display : public ::testing::Test
{
public:
    Display()
        : dummy_display{mock_egl.fake_egl_display},
          dummy_context{mock_egl.fake_egl_context},
          dummy_config{mock_egl.fake_configs[0]},
          null_display_report{mir::report::null_display_report()},
          stub_db_factory{std::make_shared<mtd::StubDisplayBuilder>()},
          stub_gl_config{std::make_shared<mtd::StubGLConfig>()},
          stub_gl_program_factory{std::make_shared<mtd::StubGLProgramFactory>()}
    {
    }

protected:
    testing::NiceMock<mtd::MockEGL> mock_egl;
    testing::NiceMock<mtd::MockGL> mock_gl;
    EGLDisplay const dummy_display;
    EGLContext const dummy_context;
    EGLConfig const dummy_config;

    std::shared_ptr<mg::DisplayReport> const null_display_report;
    std::shared_ptr<mtd::StubDisplayBuilder> const stub_db_factory;
    std::shared_ptr<mtd::StubGLConfig> const stub_gl_config;
    std::shared_ptr<mtd::StubGLProgramFactory> const stub_gl_program_factory;
};

//egl expectations are hard to tease out, we should unit-test the gl contexts instead
TEST_F(Display, creation_creates_egl_resources_properly)
{
    using namespace testing;
    EGLSurface fake_surface = reinterpret_cast<EGLSurface>(0x715);
    EGLint const expected_pbuffer_attr[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLint const expected_context_attr[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext dummy_context2 = reinterpret_cast<EGLContext>(0x4444);

    //on constrution
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .WillOnce(Return(dummy_display));
    EXPECT_CALL(mock_egl, eglInitialize(dummy_display, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(1), SetArgPointee<2>(4), Return(EGL_TRUE)));
    //display context
    EXPECT_CALL(mock_egl, eglCreateContext(
        dummy_display, _, EGL_NO_CONTEXT, mtd::AttrMatches(expected_context_attr)))
        .WillOnce(Return(dummy_context));
    EXPECT_CALL(mock_egl, eglCreatePbufferSurface(
        dummy_display, _, mtd::AttrMatches(expected_pbuffer_attr)))
        .WillOnce(Return(fake_surface));
    //primary display buffer context
    EXPECT_CALL(mock_egl, eglCreateContext(
        dummy_display, _, dummy_context, mtd::AttrMatches(expected_context_attr)))
        .WillOnce(Return(dummy_context2));
    EXPECT_CALL(mock_egl, eglCreateWindowSurface(
        dummy_display, _, Ne(nullptr), NULL))
        .WillOnce(Return(fake_surface));
    //fallback renderer could make current too
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, fake_surface, fake_surface, dummy_context2))
        .Times(AtLeast(1));
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, fake_surface, fake_surface, dummy_context))
        .Times(AtLeast(1));
    //on destruction
    EXPECT_CALL(mock_egl, eglGetCurrentContext())
        .WillRepeatedly(Return(dummy_context));
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT))
        .Times(2);
    EXPECT_CALL(mock_egl, eglDestroySurface(_,_))
        .Times(2);
    EXPECT_CALL(mock_egl, eglDestroyContext(_,_))
        .Times(2);
    EXPECT_CALL(mock_egl, eglTerminate(dummy_display))
        .Times(1);

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::disabled);
}

TEST_F(Display, selects_usable_egl_configuration)
{
    using namespace testing;
    int const incorrect_visual_id = 2;
    int const correct_visual_id = 1;
    EGLint const num_cfgs = 45;
    EGLint const expected_cfg_attr [] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_NONE };
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

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::disabled);
    EXPECT_EQ(correct_config, selected_config);
}

TEST_F(Display, respects_gl_config)
{
    using namespace testing;

    auto const mock_gl_config = std::make_shared<mtd::MockGLConfig>();
    EGLint const depth_bits{24};
    EGLint const stencil_bits{8};

    EXPECT_CALL(*mock_gl_config, depth_buffer_bits())
        .WillOnce(Return(depth_bits));
    EXPECT_CALL(*mock_gl_config, stencil_buffer_bits())
        .WillOnce(Return(stencil_bits));

    EXPECT_CALL(mock_egl,
                eglChooseConfig(
                    _,
                    AllOf(mtd::EGLConfigContainsAttrib(EGL_DEPTH_SIZE, depth_bits),
                          mtd::EGLConfigContainsAttrib(EGL_STENCIL_SIZE, stencil_bits)),
                    _,_,_));

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        mock_gl_config,
        null_display_report,
        mga::OverlayOptimization::disabled);
}

TEST_F(Display, logs_creation_events)
{
    using namespace testing;

    auto const mock_display_report = std::make_shared<mtd::MockDisplayReport>();

    EXPECT_CALL(*mock_display_report, report_successful_setup_of_native_resources())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_egl_configuration(_,_))
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_successful_egl_make_current_on_construction())
        .Times(1);
    EXPECT_CALL(*mock_display_report, report_successful_display_construction())
        .Times(1);

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        mock_display_report,
        mga::OverlayOptimization::disabled);
}

TEST_F(Display, throws_on_eglMakeCurrent_failure)
{
    using namespace testing;

    auto const mock_display_report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();

    EXPECT_CALL(*mock_display_report, report_successful_setup_of_native_resources())
        .Times(1);
    EXPECT_CALL(mock_egl, eglMakeCurrent(dummy_display, _, _, _))
        .WillOnce(Return(EGL_TRUE))
        .WillOnce(Return(EGL_TRUE))
        .WillOnce(Return(EGL_FALSE));
    EXPECT_CALL(*mock_display_report, report_successful_egl_make_current_on_construction())
        .Times(0);
    EXPECT_CALL(*mock_display_report, report_successful_display_construction())
        .Times(0);

    EXPECT_THROW({
        mga::Display display(
            stub_db_factory,
            stub_gl_program_factory,
            stub_gl_config,
            mock_display_report,
            mga::OverlayOptimization::disabled);
    }, std::runtime_error);
}

TEST_F(Display, logs_error_because_of_surface_creation_failure)
{
    using namespace testing;

    auto const mock_display_report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();

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
        mga::Display display(
            stub_db_factory,
            stub_gl_program_factory,
            stub_gl_config,
            mock_display_report,
            mga::OverlayOptimization::disabled);
    }, std::runtime_error);
}

TEST_F(Display, turns_on_db_at_construction_and_off_at_destruction)
{
    stub_db_factory->with_next_config([](mtd::MockHwcConfiguration& mock_config)
    {
        testing::InSequence seq;
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_on));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_off));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::disabled);
}

TEST_F(Display, first_power_on_is_not_fatal) //lp:1345533
{
    stub_db_factory->with_next_config([](mtd::MockHwcConfiguration& mock_config)
    {
        ON_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_on))
            .WillByDefault(testing::Throw(std::runtime_error("")));
    });

    EXPECT_NO_THROW({
        mga::Display display(
            stub_db_factory,
            stub_gl_program_factory,
            stub_gl_config,
            null_display_report,
            mga::OverlayOptimization::disabled);});
}

TEST_F(Display, catches_exceptions_when_turning_off_in_destructor)
{
    stub_db_factory->with_next_config([](mtd::MockHwcConfiguration& mock_config)
    {
        testing::InSequence seq;
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_on));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_off))
            .WillOnce(testing::Throw(std::runtime_error("")));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::disabled);
}

TEST_F(Display, configures_power_modes)
{
    stub_db_factory->with_next_config([](mtd::MockHwcConfiguration& mock_config)
    {
        using namespace testing;
        //external is not connected, so shouldn't adjust its power mode
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, _))
            .Times(0);
        testing::InSequence seq;
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_on));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_standby));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_off));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_suspend));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, mir_power_mode_off));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::disabled);

    auto configuration = display.configuration();
    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        //on by default
        if (output.id == mg::DisplayConfigurationOutputId{0})
            EXPECT_EQ(output.power_mode, mir_power_mode_on);
        output.power_mode = mir_power_mode_on;
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        if (output.id == mg::DisplayConfigurationOutputId{0})
            EXPECT_EQ(output.power_mode, mir_power_mode_on);
        output.power_mode = mir_power_mode_standby;
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        if (output.id == mg::DisplayConfigurationOutputId{0})
            EXPECT_EQ(output.power_mode, mir_power_mode_standby);
        output.power_mode = mir_power_mode_off;
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        if (output.id == mg::DisplayConfigurationOutputId{0})
            EXPECT_EQ(output.power_mode, mir_power_mode_off);
        output.power_mode = mir_power_mode_suspend;
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        if (output.id == mg::DisplayConfigurationOutputId{0})
            EXPECT_EQ(output.power_mode, mir_power_mode_suspend);
    });
}

TEST_F(Display, returns_correct_config_with_one_connected_output_at_start)
{
    using namespace testing;
    auto origin = geom::Point{0,0};
    geom::Size pixel_size{344, 111};
    geom::Size physical_size{4230, 2229};
    double vrefresh{4442.32};
    MirPixelFormat format{mir_pixel_format_abgr_8888};

    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        ON_CALL(mock_config, active_config_for(mga::DisplayName::primary))
            .WillByDefault(Return(mtd::StubDisplayConfigurationOutput{
                pixel_size, physical_size, format, vrefresh, true}));
        ON_CALL(mock_config, active_config_for(mga::DisplayName::external))
            .WillByDefault(Return(mtd::StubDisplayConfigurationOutput{
                pixel_size, physical_size, format, vrefresh, false}));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);
    auto config = display.configuration();

    std::vector<mg::DisplayConfigurationOutput> outputs;
    config->for_each_output([&](mg::DisplayConfigurationOutput const& disp_conf) {
        outputs.push_back(disp_conf);
    });
    ASSERT_EQ(2u, outputs.size());

    ASSERT_EQ(1u, outputs[0].modes.size());
    auto& disp_mode = outputs[0].modes[0];
    EXPECT_EQ(pixel_size, disp_mode.size);
    EXPECT_EQ(vrefresh, disp_mode.vrefresh_hz);
    EXPECT_EQ(mg::DisplayConfigurationOutputId{0}, outputs[0].id);
    EXPECT_EQ(mg::DisplayConfigurationCardId{0}, outputs[0].card_id);
    EXPECT_TRUE(outputs[0].connected);
    EXPECT_TRUE(outputs[0].used);
    EXPECT_EQ(origin, outputs[0].top_left);
    EXPECT_EQ(0, outputs[0].current_mode_index);
    EXPECT_EQ(physical_size, outputs[0].physical_size_mm);

    EXPECT_FALSE(outputs[1].connected);
    EXPECT_FALSE(outputs[1].used);
}

TEST_F(Display, returns_correct_config_with_external_and_primary_output_at_start)
{
    using namespace testing;
    MirPixelFormat format{mir_pixel_format_abgr_8888};
    auto origin = geom::Point{0,0};
    geom::Size primary_pixel_size{344, 111}, external_pixel_size{75,5};
    geom::Size primary_physical_size{4230, 2229}, external_physical_size{1, 22222};
    double primary_vrefresh{4442.32}, external_vrefresh{0.00001};

    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        ON_CALL(mock_config, active_config_for(mga::DisplayName::primary))
            .WillByDefault(Return(mtd::StubDisplayConfigurationOutput{
                primary_pixel_size, primary_physical_size, format, primary_vrefresh, true}));
        ON_CALL(mock_config, active_config_for(mga::DisplayName::external))
            .WillByDefault(Return(mtd::StubDisplayConfigurationOutput{mg::DisplayConfigurationOutputId{1},
                external_pixel_size, external_physical_size, format, external_vrefresh, true}));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);
    auto config = display.configuration();

    std::vector<mg::DisplayConfigurationOutput> outputs;
    config->for_each_output([&](mg::DisplayConfigurationOutput const& disp_conf) {
        outputs.push_back(disp_conf);
    });
    ASSERT_EQ(2u, outputs.size());
    ASSERT_EQ(1u, outputs[0].modes.size());
    auto& disp_mode = outputs[0].modes[0];
    EXPECT_EQ(primary_pixel_size, disp_mode.size);
    EXPECT_EQ(primary_vrefresh, disp_mode.vrefresh_hz);
    EXPECT_EQ(mg::DisplayConfigurationOutputId{0}, outputs[0].id);
    EXPECT_EQ(mg::DisplayConfigurationCardId{0}, outputs[0].card_id);
    EXPECT_TRUE(outputs[0].connected);
    EXPECT_TRUE(outputs[0].used);
    EXPECT_EQ(origin, outputs[0].top_left);
    EXPECT_EQ(0, outputs[0].current_mode_index);
    EXPECT_EQ(primary_physical_size, outputs[0].physical_size_mm);
    EXPECT_EQ(format, outputs[0].current_format);
    ASSERT_EQ(1, outputs[0].pixel_formats.size());
    EXPECT_EQ(format, outputs[0].pixel_formats[0]);

    ASSERT_EQ(1u, outputs[1].modes.size());
    disp_mode = outputs[1].modes[0];
    EXPECT_EQ(external_pixel_size, disp_mode.size);
    EXPECT_EQ(external_vrefresh, disp_mode.vrefresh_hz);
    EXPECT_EQ(mg::DisplayConfigurationOutputId{1}, outputs[1].id);
    EXPECT_EQ(mg::DisplayConfigurationCardId{0}, outputs[1].card_id);
    EXPECT_TRUE(outputs[1].connected);
    EXPECT_TRUE(outputs[1].used);
    EXPECT_EQ(origin, outputs[1].top_left);
    EXPECT_EQ(0, outputs[1].current_mode_index);
    EXPECT_EQ(external_physical_size, outputs[1].physical_size_mm);
    EXPECT_EQ(format, outputs[1].current_format);
    ASSERT_EQ(1, outputs[1].pixel_formats.size());
    EXPECT_EQ(format, outputs[1].pixel_formats[0]);
}

TEST_F(Display, incorrect_display_configure_throws)
{
    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);
    auto config = display.configuration();
    config->for_each_output([](mg::UserDisplayConfigurationOutput const& c){
        c.current_format = mir_pixel_format_invalid;
    });
    EXPECT_THROW({
        display.configure(*config);
    }, std::logic_error); 

    config->for_each_output([](mg::UserDisplayConfigurationOutput const& c){
        c.current_format = mir_pixel_format_bgr_888;
    });
    EXPECT_THROW({
        display.configure(*config);
    }, std::logic_error); 
}

//TODO: the list does not support fb target rotation yet
TEST_F(Display, display_orientation_not_supported)
{
    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);

    auto config = display.configuration();
    config->for_each_output([](mg::UserDisplayConfigurationOutput const& c){
        c.orientation = mir_orientation_left;
    });
    display.configure(*config); 

    config = display.configuration();
    config->for_each_output([](mg::UserDisplayConfigurationOutput const& c){
        if (c.id == mg::DisplayConfigurationOutputId{0})
            EXPECT_EQ(mir_orientation_left, c.orientation);
    });
}

TEST_F(Display, keeps_subscription_to_hotplug)
{
    using namespace testing;
    std::shared_ptr<void> subscription = std::make_shared<int>(3433);
    auto use_count_before = subscription.use_count();
    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        EXPECT_CALL(mock_config, subscribe_to_config_changes(_,_))
            .WillOnce(Return(subscription));
    });
    {
        mga::Display display(
            stub_db_factory,
            stub_gl_program_factory,
            stub_gl_config,
            null_display_report,
            mga::OverlayOptimization::enabled);
        EXPECT_THAT(subscription.use_count(), Gt(use_count_before));
    }
    EXPECT_THAT(subscription.use_count(), Eq(use_count_before));
}

TEST_F(Display, will_requery_display_configuration_after_hotplug)
{
    using namespace testing;
    std::shared_ptr<void> subscription = std::make_shared<int>(3433);
    std::function<void()> hotplug_fn = []{};

    mtd::StubDisplayConfigurationOutput attribs1
    {
        {33, 32},
        {31, 35},
        mir_pixel_format_abgr_8888,
        0.44,
        true,
    };
    mtd::StubDisplayConfigurationOutput attribs2
    {
        {3, 3},
        {1, 5},
        mir_pixel_format_abgr_8888,
        0.5544,
        true,
    };

    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        EXPECT_CALL(mock_config, subscribe_to_config_changes(_,_))
            .WillOnce(DoAll(SaveArg<0>(&hotplug_fn), Return(subscription)));

        EXPECT_CALL(mock_config, active_config_for(mga::DisplayName::primary))
            .Times(2)
            .WillOnce(testing::Return(attribs1))
            .WillOnce(testing::Return(attribs2));
        EXPECT_CALL(mock_config, active_config_for(mga::DisplayName::external))
            .Times(2)
            .WillOnce(testing::Return(attribs1))
            .WillOnce(testing::Return(attribs2));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);

    auto config = display.configuration();
    config->for_each_output([&](mg::UserDisplayConfigurationOutput const& c){
        EXPECT_THAT(c.modes[c.current_mode_index].size, Eq(attribs1.modes[attribs1.current_mode_index].size));
    });

    hotplug_fn();
    config = display.configuration();
    config = display.configuration();
    config->for_each_output([&](mg::UserDisplayConfigurationOutput const& c){
        EXPECT_THAT(c.modes[c.current_mode_index].size, Eq(attribs2.modes[attribs2.current_mode_index].size));
    });
}

TEST_F(Display, returns_correct_dbs_with_external_and_primary_output_at_start)
{
    using namespace testing;
    std::function<void()> hotplug_fn = []{};
    bool external_connected = true;
    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        ON_CALL(mock_config, active_config_for(mga::DisplayName::primary))
            .WillByDefault(Return(mtd::StubDisplayConfigurationOutput{
                mg::DisplayConfigurationOutputId{0}, {20,20}, {4,4}, mir_pixel_format_abgr_8888, 50.0f, true}));

        ON_CALL(mock_config, active_config_for(mga::DisplayName::external))
            .WillByDefault(Invoke([&](mga::DisplayName)
            {
                return mtd::StubDisplayConfigurationOutput{mg::DisplayConfigurationOutputId{1},
                    {20,20}, {4,4}, mir_pixel_format_abgr_8888, 50.0f, external_connected};
            }));
        EXPECT_CALL(mock_config, subscribe_to_config_changes(_,_))
            .WillOnce(DoAll(SaveArg<0>(&hotplug_fn), Return(std::make_shared<char>('2'))));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);

    auto group_count = 0;
    auto db_count = 0;
    auto db_group_counter = [&](mg::DisplaySyncGroup& group) {
        group_count++;
        group.for_each_display_buffer([&](mg::DisplayBuffer&) {db_count++;});
    };
    display.for_each_display_sync_group(db_group_counter);
    EXPECT_THAT(group_count, Eq(1));
    EXPECT_THAT(db_count, Eq(2));

    //hotplug external away
    external_connected = false;
    hotplug_fn();

    group_count = 0;
    db_count = 0;
    display.for_each_display_sync_group(db_group_counter);
    EXPECT_THAT(group_count, Eq(1));
    EXPECT_THAT(db_count, Eq(1));

    //hotplug external back 
    external_connected = true;
    hotplug_fn();

    group_count = 0;
    db_count = 0;
    display.for_each_display_sync_group(db_group_counter);
    EXPECT_THAT(group_count, Eq(1));
    EXPECT_THAT(db_count, Eq(2));
}

TEST_F(Display, turns_external_display_on_with_hotplug)
{
    using namespace testing;
    std::function<void()> hotplug_fn = []{};
    bool external_connected = true;
    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        EXPECT_CALL(mock_config, subscribe_to_config_changes(_,_))
            .WillOnce(DoAll(SaveArg<0>(&hotplug_fn), Return(std::make_shared<char>('2'))));
        ON_CALL(mock_config, active_config_for(mga::DisplayName::primary))
            .WillByDefault(Return(mtd::StubDisplayConfigurationOutput{
                {20,20}, {4,4}, mir_pixel_format_abgr_8888, 50.0f, true}));
        ON_CALL(mock_config, active_config_for(mga::DisplayName::external))
            .WillByDefault(Invoke([&](mga::DisplayName)
            {
                return mtd::StubDisplayConfigurationOutput{
                    {20,20}, {4,4}, mir_pixel_format_abgr_8888, 50.0f, external_connected};
            }));


        testing::InSequence seq;
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, _));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, mir_power_mode_on));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, mir_power_mode_on));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, _));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, mir_power_mode_off));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);

    //hotplug external away
    external_connected = false;
    hotplug_fn();
    display.for_each_display_sync_group([](mg::DisplaySyncGroup&){});

    //hotplug external back 
    external_connected = true;
    hotplug_fn();
    display.for_each_display_sync_group([](mg::DisplaySyncGroup&){});
}

TEST_F(Display, configures_external_display)
{
    using namespace testing;
    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        ON_CALL(mock_config, active_config_for(mga::DisplayName::external))
            .WillByDefault(Return(mtd::StubDisplayConfigurationOutput{
                mg::DisplayConfigurationOutputId{1}, {0,0}, {0,0}, mir_pixel_format_abgr_8888, 0.0, true}));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::primary, _))
            .Times(AnyNumber());
        InSequence seq;
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, mir_power_mode_on));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, mir_power_mode_off));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, mir_power_mode_suspend));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, mir_power_mode_on));
        EXPECT_CALL(mock_config, power_mode(mga::DisplayName::external, mir_power_mode_off));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);

    auto configuration = display.configuration();
    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        output.power_mode = mir_power_mode_off;
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        output.power_mode = mir_power_mode_suspend;
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        output.power_mode = mir_power_mode_on;
    });
    display.configure(*configuration);

    configuration->for_each_output([&](mg::UserDisplayConfigurationOutput& output) {
        output.power_mode = mir_power_mode_off;
    });
    display.configure(*configuration);
}

TEST_F(Display, reports_vsync)
{
    using namespace testing;
    std::function<void(mga::DisplayName)> vsync_fn = [](mga::DisplayName){};
    auto report = std::make_shared<NiceMock<mtd::MockDisplayReport>>();
    EXPECT_CALL(*report, report_vsync(_));
    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        EXPECT_CALL(mock_config, subscribe_to_config_changes(_,_))
            .WillOnce(DoAll(SaveArg<1>(&vsync_fn), Return(std::make_shared<char>('2'))));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        report,
        mga::OverlayOptimization::enabled);

    vsync_fn(mga::DisplayName::primary);
}

TEST_F(Display, reports_correct_card_information)
{
    using namespace testing;
    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);

    int num_cards = 0;
    display.configuration()->for_each_card(
        [&](mg::DisplayConfigurationCard const& config)
        {
            EXPECT_THAT(config.max_simultaneous_outputs, Eq(2));
            num_cards++;
        });
    EXPECT_THAT(num_cards, Eq(1));
}

TEST_F(Display, can_configure_positioning_of_dbs)
{
    using namespace testing;
    auto new_location = geom::Point{493,999};
    auto another_new_location = geom::Point{540,221};
    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);

    auto config = display.configuration();
    config->for_each_output([&](mg::UserDisplayConfigurationOutput& disp_conf) {
        disp_conf.top_left = new_location;
    });
    display.configure(*config);

    config = display.configuration();
    config->for_each_output([&](mg::UserDisplayConfigurationOutput& disp_conf) {
        EXPECT_THAT(disp_conf.top_left, Eq(new_location));
        disp_conf.top_left = another_new_location; 
    });

    config->for_each_output([&](mg::DisplayConfigurationOutput const& disp_conf) {
        EXPECT_THAT(disp_conf.top_left, Eq(another_new_location));
    });
}

//test for lp:1471858
TEST_F(Display, applying_orientation_after_hotplug)
{
    using namespace testing;
    std::function<void()> hotplug_fn = []{};
    bool external_connected = false;
    MirOrientation const orientation = mir_orientation_left;
    stub_db_factory->with_next_config([&](mtd::MockHwcConfiguration& mock_config)
    {
        ON_CALL(mock_config, active_config_for(mga::DisplayName::primary))
            .WillByDefault(Return(mtd::StubDisplayConfigurationOutput{
                mg::DisplayConfigurationOutputId{0}, {20,20}, {4,4}, mir_pixel_format_abgr_8888, 50.0f, true}));
        ON_CALL(mock_config, active_config_for(mga::DisplayName::external))
            .WillByDefault(Invoke([&](mga::DisplayName)
            {
                return mtd::StubDisplayConfigurationOutput{mg::DisplayConfigurationOutputId{1},
                    {20,20}, {4,4}, mir_pixel_format_abgr_8888, 50.0f, external_connected};
            }));
        EXPECT_CALL(mock_config, subscribe_to_config_changes(_,_))
            .WillOnce(DoAll(SaveArg<0>(&hotplug_fn), Return(std::make_shared<char>('2'))));
    });

    mga::Display display(
        stub_db_factory,
        stub_gl_program_factory,
        stub_gl_config,
        null_display_report,
        mga::OverlayOptimization::enabled);

    //hotplug external back 
    external_connected = true;
    hotplug_fn();

    auto config = display.configuration();
    config->for_each_output([orientation](mg::UserDisplayConfigurationOutput& output) {
        output.orientation = orientation;
    });
    display.configure(*config);
    display.for_each_display_sync_group([orientation](mg::DisplaySyncGroup& group) {
        group.for_each_display_buffer([orientation](mg::DisplayBuffer& db) {
            EXPECT_THAT(db.orientation(), Eq(orientation)); 
        });
    });
}
