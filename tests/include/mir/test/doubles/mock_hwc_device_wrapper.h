/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#ifndef MIR_TEST_DOUBLES_MOCK_HWC_DEVICE_WRAPPER_H_
#define MIR_TEST_DOUBLES_MOCK_HWC_DEVICE_WRAPPER_H_

#include "src/platforms/android/server/hwc_wrapper.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockHWCDeviceWrapper : public graphics::android::HwcWrapper
{
    MockHWCDeviceWrapper()
    {
        using namespace testing;
        using graphics::android::ConfigId;
        ON_CALL(*this, display_configs(_))
            .WillByDefault(Return(std::vector<ConfigId>{ConfigId{34}}));
    }
    MOCK_CONST_METHOD1(prepare, void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&));
    MOCK_CONST_METHOD1(set, void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&));
    MOCK_CONST_METHOD1(vsync_signal_on, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(vsync_signal_off, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(display_on, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(display_off, void(graphics::android::DisplayName));
    MOCK_METHOD4(subscribe_to_events, void(void const*,
        std::function<void(graphics::android::DisplayName, std::chrono::nanoseconds)> const&,
        std::function<void(graphics::android::DisplayName, bool)> const&,
        std::function<void()> const&));
    MOCK_METHOD1(unsubscribe_from_events_, void(void const*));
    void unsubscribe_from_events(void const* id) noexcept
    {
        unsubscribe_from_events_(id);
    }
    MOCK_CONST_METHOD1(display_configs, std::vector<graphics::android::ConfigId>(graphics::android::DisplayName));
    MOCK_CONST_METHOD4(display_attributes, int(
        graphics::android::DisplayName, graphics::android::ConfigId, uint32_t const*, int32_t*));
    MOCK_CONST_METHOD2(power_mode, void(graphics::android::DisplayName, graphics::android::PowerMode));
    MOCK_CONST_METHOD1(has_active_config, bool(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(active_config_for, graphics::android::ConfigId(graphics::android::DisplayName));
    MOCK_CONST_METHOD2(set_active_config, void(graphics::android::DisplayName name, graphics::android::ConfigId id));

};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_HWC_DEVICE_WRAPPER_H_ */
