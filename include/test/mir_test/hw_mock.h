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

#include <memory>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h> 
#include <gmock/gmock.h>
#include "mir_test_doubles/mock_android_alloc_device.h"

namespace mir
{
namespace test
{

class HWCComposer1Interface
{

};

class MockHWCComposerDevice1 : public HWCComposer1Interface,
                               public hwc_composer_device_1
{
public:
    MockHWCComposerDevice1()
    {
        common.version = HWC_DEVICE_API_VERSION_1_1;
    } 
};

typedef struct hw_module_t hw_module;
class hardware_module_mock : public hw_module
{
public:
    hardware_module_mock(hw_device_t* const& device)
        : mock_ad(device)
    { 
        gr_methods.open = hw_open;
        methods = &gr_methods; 
    }
    ~hardware_module_mock()
    {
    } 

    static int hw_open(const struct hw_module_t* module, const char*, struct hw_device_t** device)
    {
        hardware_module_mock* self = (hardware_module_mock*) module;
        self->mock_ad->close = hw_close;
        *device = (hw_device_t*) self->mock_ad;
        return 0;
    }

    static int hw_close(struct hw_device_t*)
    {
        return 0;
    }

    hw_module_methods_t gr_methods;
    hw_device_t* mock_ad;
};


class HardwareAccessMock
{
public:
    HardwareAccessMock();
    ~HardwareAccessMock();

    MOCK_METHOD2(hw_get_module, int(const char *id, const struct hw_module_t**));

    std::shared_ptr<alloc_device_t> ad; 
    std::shared_ptr<hwc_composer_device_1> hd;
 
    std::shared_ptr<hardware_module_mock> grmock;
    std::shared_ptr<hardware_module_mock> hwmock;
};

}
}

#endif /* MIR_TEST_DOUBLES_MOCK_ANDROID_HWC_DEVICE_H_ */
