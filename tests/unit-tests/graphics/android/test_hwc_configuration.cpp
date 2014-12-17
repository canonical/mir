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

#include "src/platforms/android/hwc_configuration.h"
#include "mir_test_doubles/mock_hwc_device_wrapper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>

namespace mga = mir::graphics::android;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

struct HwcConfiguration : public testing::Test
{
    std::shared_ptr<mtd::MockHWCDeviceWrapper> mock_hwc_wrapper{
        std::make_shared<testing::NiceMock<mtd::MockHWCDeviceWrapper>>()};
    mga::HwcBlankingControl config{mock_hwc_wrapper};
    mga::DisplayName display{mga::DisplayName::primary};
};

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

TEST_F(HwcConfiguration, queries_connected_primary_display_properties)
{
    using namespace testing;
    geom::Size px_size {343, 254};

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
                        default:
                            break;
                    }
                    i++;
                }
                //the attribute list should be at least this long, as some qcom drivers always deref attribute_list[5]
                EXPECT_EQ(5, i);
            }));

    auto vrefresh_hz = 1000.0 / vrefresh_period.count();
    auto attribs = config.active_attribs_for(display);
    EXPECT_THAT(attribs.pixel_size, Eq(px_size));
    EXPECT_THAT(attribs.vrefresh_hz, Eq(vrefresh_hz));
    EXPECT_TRUE(attribs.connected);
}

//the primary display should not be disconnected, but this is how to tell if the external one is
TEST_F(HwcConfiguration, test_hwc_device_display_config_failure_throws)
{
    using namespace testing;
    ON_CALL(*mock_hwc_wrapper, display_configs(_))
        .WillByDefault(Return(std::vector<mga::ConfigId>{}));

    EXPECT_THROW({
        config.active_attribs_for(mga::DisplayName::primary);
    }, std::runtime_error);
    auto external_attribs = config.active_attribs_for(mga::DisplayName::external);
    EXPECT_THAT(external_attribs.pixel_size, Eq(geom::Size{0,0}));
    EXPECT_THAT(external_attribs.vrefresh_hz, Eq(0.0));
    EXPECT_FALSE(external_attribs.connected);
}

TEST_F(HwcConfiguration, no_fpe_from_malformed_refresh)
{
    using namespace testing;
    EXPECT_CALL(*mock_hwc_wrapper, display_attributes( _, _, _, _))
            .WillOnce(Invoke([]
            (mga::DisplayName, mga::ConfigId, uint32_t const* attribute_list, int32_t* values)
            {
                int i = 0;
                while(attribute_list[i] != HWC_DISPLAY_NO_ATTRIBUTE)
                {
                    switch(attribute_list[i])
                    {
                        case HWC_DISPLAY_VSYNC_PERIOD:
                            values[i] = 0;
                            break;
                        default:
                            break;
                    }
                    i++;
                }
            }));
    auto attribs = config.active_attribs_for(mga::DisplayName::external);
    EXPECT_THAT(attribs.vrefresh_hz, Eq(0.0f));
}
