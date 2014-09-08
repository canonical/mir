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
    MOCK_CONST_METHOD1(prepare, void(hwc_display_contents_1_t&));
    MOCK_CONST_METHOD1(set, void(hwc_display_contents_1_t&));
    MOCK_CONST_METHOD1(register_hooks, void(hwc_procs_t*));
    MOCK_CONST_METHOD0(vsync_signal_on, void());
    MOCK_CONST_METHOD0(vsync_signal_off, void());
    MOCK_CONST_METHOD0(display_on, void());
    MOCK_CONST_METHOD0(display_off, void());
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_HWC_DEVICE_WRAPPER_H_ */
