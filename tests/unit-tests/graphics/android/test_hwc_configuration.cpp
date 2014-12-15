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

#include "src/platform/graphics/android/hwc_configuration.h"
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
        std::make_shared<mtd::MockHWCDeviceWrapper>()};
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

TEST_F(HwcConfiguration, turns_screen_off)
{
    testing::InSequence seq;
    EXPECT_CALL(*mock_hwc_wrapper, vsync_signal_off(display));
    EXPECT_CALL(*mock_hwc_wrapper, display_off(display));
    EXPECT_CALL(*mock_hwc_wrapper, vsync_signal_off(display));
    EXPECT_CALL(*mock_hwc_wrapper, display_off(display));
    EXPECT_CALL(*mock_hwc_wrapper, vsync_signal_off(display));
    EXPECT_CALL(*mock_hwc_wrapper, display_off(display));
    config.power_mode(display, mir_power_mode_off);

    //HWC version 1.3 and prior do not support anything more than on and off.
    //translate this into blanking the screen
    config.power_mode(display, mir_power_mode_suspend);
    config.power_mode(display, mir_power_mode_standby);
}

TEST_F(HwcConfiguration, queries_connected_primary_display_properties)
{
    using namespace testing;
    geom::Size px_size {343, 254};
    geom::Size dpi_mm {343, 254};
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
                            values[i] = dpi_mm.width.as_int();
                            break;
                        case HWC_DISPLAY_DPI_Y:
                            values[i] = dpi_mm.height.as_int();
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
    auto attribs = config.display_attribs(display);
    EXPECT_THAT(attribs.pixel_size, Eq(px_size));
    EXPECT_THAT(attribs.dpi_mm, Eq(dpi_mm));
    EXPECT_THAT(attribs.vrefresh_hz, Eq(vrefresh_hz));
    EXPECT_TRUE(attribs.connected);
}

#if 0
//the primary display should not be disconnected
TEST_F(HwcConfiguration, test_hwc_device_display_config_failure_throws)
{
    using namespace testing;
    EXPECT_CALL(*mock_device, getDisplayConfigs_interface(mock_device.get(),HWC_DISPLAY_PRIMARY,_,_))
        .WillOnce(Return(-1));

    EXPECT_THROW({
        mga::RealHwcWrapper wrapper(mock_device, mock_report);
        wrapper.display_attribs(mga::DisplayName::primary);
    }, std::runtime_error);
}
#endif
