/*
 * Copyright © 2014 Canonical Ltd.
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

#include "src/platforms/android/server/hwc_configuration.h"
#include "mir/test/doubles/mock_hwc_device_wrapper.h"
#include "mir/test/doubles/mock_egl.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>

namespace mga = mir::graphics::android;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

struct HwcConfiguration : public testing::Test
{
    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<mtd::MockHWCDeviceWrapper> mock_hwc_wrapper{
        std::make_shared<testing::NiceMock<mtd::MockHWCDeviceWrapper>>()};
    mga::DisplayName display{mga::DisplayName::primary};
    mga::HwcBlankingControl config{mock_hwc_wrapper};
    mga::HwcPowerModeControl power_mode_config{mock_hwc_wrapper};
};

TEST_F(HwcConfiguration, fb_format_selection)
{
    using namespace testing;
    Mock::VerifyAndClearExpectations(&mock_egl);
    EGLint const expected_egl_config_attr [] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_FRAMEBUFFER_TARGET_ANDROID, EGL_TRUE,
        EGL_NONE
    };

    int visual_id = HAL_PIXEL_FORMAT_BGRA_8888;
    EGLDisplay fake_display = reinterpret_cast<EGLDisplay>(0x11235813);
    EGLConfig fake_egl_config = reinterpret_cast<EGLConfig>(0x44);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .InSequence(seq)
        .WillOnce(Return(fake_display));
    EXPECT_CALL(mock_egl, eglInitialize(fake_display,_,_))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglChooseConfig(fake_display,mtd::AttrMatches(expected_egl_config_attr),_,1,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<2>(fake_egl_config), SetArgPointee<4>(1), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglGetConfigAttrib(fake_display, fake_egl_config, EGL_NATIVE_VISUAL_ID, _))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<3>(visual_id), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglTerminate(fake_display))
        .InSequence(seq);

    mga::HwcBlankingControl hwc_config{mock_hwc_wrapper};
    EXPECT_EQ(mir_pixel_format_argb_8888, hwc_config.active_config_for(mga::DisplayName::primary).current_format);
}

//not all hwc implementations give a hint about their framebuffer formats in their configuration.
//prefer abgr_8888 if we can't figure things out
TEST_F(HwcConfiguration, format_selection_failure_default)
{
    using namespace testing;
    Mock::VerifyAndClearExpectations(&mock_egl);
    EGLDisplay fake_display = reinterpret_cast<EGLDisplay>(0x11235813);

    Sequence seq;
    EXPECT_CALL(mock_egl, eglGetDisplay(EGL_DEFAULT_DISPLAY))
        .InSequence(seq)
        .WillOnce(Return(fake_display));
    EXPECT_CALL(mock_egl, eglInitialize(fake_display,_,_))
        .InSequence(seq);
    EXPECT_CALL(mock_egl, eglChooseConfig(_,_,_,_,_))
        .InSequence(seq)
        .WillOnce(DoAll(SetArgPointee<4>(0), Return(EGL_TRUE)));
    EXPECT_CALL(mock_egl, eglTerminate(fake_display))
        .InSequence(seq);

    mga::HwcBlankingControl hwc_config{mock_hwc_wrapper};
    EXPECT_EQ(mir_pixel_format_abgr_8888, hwc_config.active_config_for(mga::DisplayName::primary).current_format);
}

TEST_F(HwcConfiguration, turns_screen_on)
{
    testing::InSequence seq;
    EXPECT_CALL(*mock_hwc_wrapper, display_on(display));
    EXPECT_CALL(*mock_hwc_wrapper, vsync_signal_on(display));
    config.power_mode(display, mir_power_mode_on);
}

TEST_F(HwcConfiguration, turns_screen_off_for_off_suspend_and_standby)
{
    testing::InSequence seq;
    EXPECT_CALL(*mock_hwc_wrapper, vsync_signal_off(display));
    EXPECT_CALL(*mock_hwc_wrapper, display_off(display));
    EXPECT_CALL(*mock_hwc_wrapper, display_on(display));
    EXPECT_CALL(*mock_hwc_wrapper, vsync_signal_on(display));
    EXPECT_CALL(*mock_hwc_wrapper, vsync_signal_off(display));
    EXPECT_CALL(*mock_hwc_wrapper, display_off(display));
    config.power_mode(display, mir_power_mode_off);

    //HWC version 1.3 and prior do not support anything more than on and off.
    config.power_mode(display, mir_power_mode_suspend);
    config.power_mode(display, mir_power_mode_off);
    config.power_mode(display, mir_power_mode_standby);
    config.power_mode(display, mir_power_mode_on);
    config.power_mode(display, mir_power_mode_standby);
    //translate this into blanking the screen
    config.power_mode(display, mir_power_mode_suspend);
    config.power_mode(display, mir_power_mode_standby);
}

TEST_F(HwcConfiguration, translates_mir_power_mode_to_hwc_power_mode)
{
    testing::InSequence seq;

    EXPECT_CALL(*mock_hwc_wrapper, power_mode(display, mga::PowerMode::off));
    EXPECT_CALL(*mock_hwc_wrapper, power_mode(display, mga::PowerMode::doze_suspend));
    EXPECT_CALL(*mock_hwc_wrapper, power_mode(display, mga::PowerMode::doze));
    EXPECT_CALL(*mock_hwc_wrapper, power_mode(display, mga::PowerMode::normal));

    power_mode_config.power_mode(display, mir_power_mode_off);
    power_mode_config.power_mode(display, mir_power_mode_suspend);
    power_mode_config.power_mode(display, mir_power_mode_standby);
    power_mode_config.power_mode(display, mir_power_mode_on);
}

TEST_F(HwcConfiguration, queries_connected_primary_display_properties)
{
    using namespace testing;
    auto android_reported_dpi_x = 390000u;
    auto android_reported_dpi_y = 400000u;
    geom::Size px_size {768, 1280};
    geom::Size mm_size {50, 81};

    std::vector<mga::ConfigId> hwc_config {mga::ConfigId{0xA1}, mga::ConfigId{0xBEE}};
    std::chrono::milliseconds vrefresh_period {16};

    EXPECT_CALL(*mock_hwc_wrapper, display_configs(display))
            .WillOnce(Return(hwc_config));
    EXPECT_CALL(*mock_hwc_wrapper, display_attributes(display, hwc_config[0], _, _))
            .WillOnce(Invoke([&]
            (mga::DisplayName, mga::ConfigId, uint32_t const* attribute_list, int32_t* values)
            {
                int i = 0;
                while(attribute_list[i] != HWC_DISPLAY_NO_ATTRIBUTE)
                {
                    switch(attribute_list[i])
                    {
                        case HWC_DISPLAY_WIDTH:
                            values[i] = px_size.width.as_int();
                            break;
                        case HWC_DISPLAY_HEIGHT:
                            values[i] = px_size.height.as_int();
                            break;
                        case HWC_DISPLAY_VSYNC_PERIOD:
                            values[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(vrefresh_period).count();
                            break;
                        case HWC_DISPLAY_DPI_X:
                            values[i] = android_reported_dpi_x;
                            break;
                        case HWC_DISPLAY_DPI_Y:
                            values[i] = android_reported_dpi_y;
                            break;
                        default:
                            break;
                    }
                    i++;
                }
                //the attribute list should be at least this long, as some qcom drivers always deref attribute_list[5]
                EXPECT_EQ(5, i);
                return 0;
            }));

    auto vrefresh_hz = 1000.0 / vrefresh_period.count();
    auto attribs = config.active_config_for(display);
    ASSERT_THAT(attribs.modes.size(), Eq(1));
    EXPECT_THAT(attribs.modes[0].size, Eq(px_size));
    EXPECT_THAT(attribs.modes[0].vrefresh_hz, Eq(vrefresh_hz));
    EXPECT_THAT(attribs.physical_size_mm, Eq(mm_size));
    EXPECT_TRUE(attribs.connected);
    EXPECT_TRUE(attribs.used);
}

//the primary display should not be disconnected, but this is how to tell if the external one is
TEST_F(HwcConfiguration, test_hwc_device_display_config_failure_throws)
{
    using namespace testing;
    ON_CALL(*mock_hwc_wrapper, display_configs(_))
        .WillByDefault(Return(std::vector<mga::ConfigId>{}));

    EXPECT_THROW({
        config.active_config_for(mga::DisplayName::primary);
    }, std::runtime_error);
    auto external_attribs = config.active_config_for(mga::DisplayName::external);
    EXPECT_THAT(external_attribs.modes.size(), Eq(0));
    EXPECT_FALSE(external_attribs.connected);
    EXPECT_FALSE(external_attribs.used);
}

//some devices (bq) only report an error later in the display attributes call, make sure to report disconnected on error to this call. 
TEST_F(HwcConfiguration, display_attributes_failure_indicates_problem_for_primary_disconnect_for_secondary)
{
    using namespace testing;
    ON_CALL(*mock_hwc_wrapper, display_attributes(_,_,_,_))
        .WillByDefault(Return(-22));

    EXPECT_THROW({
        config.active_config_for(mga::DisplayName::primary);
    }, std::runtime_error);
    auto external_attribs = config.active_config_for(mga::DisplayName::external);
    EXPECT_THAT(external_attribs.modes.size(), Eq(0));
    EXPECT_FALSE(external_attribs.connected);
    EXPECT_FALSE(external_attribs.used);
}

TEST_F(HwcConfiguration, no_fpe_from_malformed_refresh)
{
    using namespace testing;
    EXPECT_CALL(*mock_hwc_wrapper, display_attributes(_,_,_,_))
            .WillOnce(Invoke([]
            (mga::DisplayName, mga::ConfigId, uint32_t const* attribute_list, int32_t* values)
            {
                int i = 0;
                while(attribute_list[i] != HWC_DISPLAY_NO_ATTRIBUTE)
                    values[i++] = 0;
                return 0;
            }));
    auto attribs = config.active_config_for(mga::DisplayName::external);
    EXPECT_THAT(attribs.modes[attribs.current_mode_index].vrefresh_hz, Eq(0.0f));
}

TEST_F(HwcConfiguration, no_fpe_from_malformed_dpi)
{
    using namespace testing;
    EXPECT_CALL(*mock_hwc_wrapper, display_attributes(_,_,_,_))
            .WillOnce(Invoke([]
            (mga::DisplayName, mga::ConfigId, uint32_t const* attribute_list, int32_t* values)
            {
                int i = 0;
                while(attribute_list[i] != HWC_DISPLAY_NO_ATTRIBUTE)
                    values[i++] = 0;
                return 0;
            }));
    auto attribs = config.active_config_for(mga::DisplayName::external);
    EXPECT_THAT(attribs.physical_size_mm, Eq(geom::Size{0,0}));
}

TEST_F(HwcConfiguration, subscribes_to_hotplug_and_vsync)
{
    using namespace testing;
    std::function<void(mga::DisplayName, bool)> hotplug_fn([](mga::DisplayName, bool){});
    std::function<void(mga::DisplayName, std::chrono::nanoseconds)> vsync_fn(
        [](mga::DisplayName, std::chrono::nanoseconds){});
    EXPECT_CALL(*mock_hwc_wrapper, subscribe_to_events(_,_,_,_))
        .WillOnce(DoAll(SaveArg<1>(&vsync_fn), SaveArg<2>(&hotplug_fn)));
    EXPECT_CALL(*mock_hwc_wrapper, unsubscribe_from_events_(_));

    unsigned int hotplug_call_count{0};
    unsigned int vsync_call_count{0};
    auto subscription = config.subscribe_to_config_changes(
        [&]{ hotplug_call_count++; }, [&](mga::DisplayName){ vsync_call_count++; });
    hotplug_fn(mga::DisplayName::primary, true);
    hotplug_fn(mga::DisplayName::primary, true);
    vsync_fn(mga::DisplayName::primary, std::chrono::nanoseconds(33));
    EXPECT_THAT(hotplug_call_count, Eq(2));
    EXPECT_THAT(vsync_call_count, Eq(1));
}

TEST_F(HwcConfiguration, sets_active_config_when_needed)
{
    using namespace testing;

    std::vector<mga::ConfigId> config_ids{mga::ConfigId{0xA1}, mga::ConfigId{0xBEE}};
    mga::DisplayName display_name{mga::DisplayName::primary};

    EXPECT_CALL(*mock_hwc_wrapper, display_configs(display))
        .WillOnce(Return(config_ids));
    EXPECT_CALL(*mock_hwc_wrapper, has_active_config(display_name))
        .WillOnce(Return(false));
    EXPECT_CALL(*mock_hwc_wrapper, set_active_config(display_name, config_ids[0]));

    power_mode_config.active_config_for(display_name);
}

TEST_F(HwcConfiguration, uses_given_active_config_id)
{
    using namespace testing;

    std::vector<mga::ConfigId> config_ids{mga::ConfigId{0xA1}, mga::ConfigId{0xBEE}};
    mga::DisplayName display_name{mga::DisplayName::external};

    EXPECT_CALL(*mock_hwc_wrapper, display_configs(display_name))
        .WillOnce(Return(config_ids));
    EXPECT_CALL(*mock_hwc_wrapper, has_active_config(display_name))
        .WillOnce(Return(true));
    EXPECT_CALL(*mock_hwc_wrapper, active_config_for(display_name))
        .WillOnce(Return(config_ids[1]));
    EXPECT_CALL(*mock_hwc_wrapper, display_attributes(display_name, config_ids[1], _, _));

    power_mode_config.active_config_for(display_name);
}
