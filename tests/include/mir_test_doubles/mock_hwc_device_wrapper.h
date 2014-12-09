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

#include "src/platform/graphics/android/hwc_wrapper.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct MockHWCDeviceWrapper : public graphics::android::HwcWrapper
{
    MOCK_CONST_METHOD1(prepare, void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&));
    MOCK_CONST_METHOD1(set, void(std::array<hwc_display_contents_1_t*, HWC_NUM_DISPLAY_TYPES> const&));
    MOCK_METHOD1(register_hooks, void(std::shared_ptr<graphics::android::HWCCallbacks> const&));
    MOCK_CONST_METHOD1(vsync_signal_on, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(vsync_signal_off, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(display_on, void(graphics::android::DisplayName));
    MOCK_CONST_METHOD1(display_off, void(graphics::android::DisplayName));
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_HWC_DEVICE_WRAPPER_H_ */
