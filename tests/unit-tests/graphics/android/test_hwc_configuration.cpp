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

namespace mga = mir::graphics::android;
namespace mtd = mir::test::doubles;
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
