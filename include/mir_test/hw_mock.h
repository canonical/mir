/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_TEST_DOUBLES_MOCK_ANDROID_HWC_DEVICE_H_
#define MIR_TEST_DOUBLES_MOCK_ANDROID_HWC_DEVICE_H_

#include <hardware/hardware.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{

class HardwareAccessMock
{
public:
    HardwareAccessMock();
    ~HardwareAccessMock();

    MOCK_METHOD2(hw_get_module, int(const char *id, const struct hw_module_t**));

    struct hw_device_t *fake_hwc_device;
    struct hw_device_t *fake_gr_device;

    hw_module_t fake_hw_gr_module;
    hw_module_methods_t gr_methods;

    hw_module_t fake_hw_hwc_module;
    hw_module_methods_t hwc_methods; 
};

}
}

#endif /* MIR_TEST_DOUBLES_MOCK_ANDROID_HWC_DEVICE_H_ */
