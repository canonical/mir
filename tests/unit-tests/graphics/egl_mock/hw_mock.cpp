/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_test/hw_mock.h"
#include "mir_test_doubles/mock_hwc_composer_device_1.h"

#include <hardware/gralloc.h>
#include <system/window.h>
#include <memory>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;

mt::HardwareModuleStub::HardwareModuleStub(hw_device_t& device)
    : mock_hw_device(device)
{ 
    gr_methods.open = hw_open;
    methods = &gr_methods; 
}

int mt::HardwareModuleStub::hw_open(const struct hw_module_t* module, const char*, struct hw_device_t** device)
{
    auto self = static_cast<HardwareModuleStub const*>(module);
    self->mock_hw_device.close = hw_close;
    *device = static_cast<hw_device_t*>(&self->mock_hw_device);
    return 0;
}

int mt::HardwareModuleStub::hw_close(struct hw_device_t*)
{
    return 0;
}

namespace
{
mt::HardwareAccessMock* global_hw_mock = NULL;
}

mt::HardwareAccessMock::HardwareAccessMock()
{
    using namespace testing;
    assert(global_hw_mock == NULL && "Only one mock object per process is allowed");
    global_hw_mock = this;

    mock_alloc_device = std::make_shared<mtd::MockAllocDevice>();
    mock_gralloc_module = std::make_shared<mt::HardwareModuleStub>(mock_alloc_device->common);

    mock_hwc_device = std::make_shared<mtd::MockHWCComposerDevice1>();
    mock_hwc_module = std::make_shared<mt::HardwareModuleStub>(mock_hwc_device->common);

    ON_CALL(*this, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_gralloc_module.get()), Return(0)));
    ON_CALL(*this, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_hwc_module.get()), Return(0)));
}

mt::HardwareAccessMock::~HardwareAccessMock()
{
    global_hw_mock = NULL;
}

int hw_get_module(const char *id, const struct hw_module_t **module)
{
    if (!global_hw_mock)
    {
        ADD_FAILURE_AT(__FILE__,__LINE__); \
        return -1;
    }
    int rc =  global_hw_mock->hw_get_module(id, module);
    return rc;
}
