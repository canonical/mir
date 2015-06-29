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

#include "mir/test/doubles/mock_android_hw.h"
#include "mir/test/doubles/mock_hwc_composer_device_1.h"

#include <atomic>
#include <hardware/gralloc.h>
#include <system/window.h>
#include <memory>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
mtd::HardwareAccessMock* global_mock_android_hw = NULL;
std::atomic<int> open_count;
}

mtd::HardwareModuleStub::HardwareModuleStub(hw_device_t& device)
    : mock_hw_device(device)
{
    gr_methods.open = hw_open;
    methods = &gr_methods;
}

int mtd::HardwareModuleStub::hw_open(const struct hw_module_t* module, const char*, struct hw_device_t** device)
{
    auto self = static_cast<HardwareModuleStub const*>(module);
    self->mock_hw_device.close = hw_close;
    *device = static_cast<hw_device_t*>(&self->mock_hw_device);
    open_count++;
    return 0;
}

int mtd::HardwareModuleStub::hw_close(struct hw_device_t*)
{
    open_count--;
    return 0;
}

mtd::FailingHardwareModuleStub::FailingHardwareModuleStub()
{
    gr_methods.open = hw_open;
    methods = &gr_methods;
}

int mtd::FailingHardwareModuleStub::hw_open(const struct hw_module_t*, const char*, struct hw_device_t**)
{
    return -1;
}

int mtd::FailingHardwareModuleStub::hw_close(struct hw_device_t*)
{
    return 0;
}

mtd::HardwareAccessMock::HardwareAccessMock()
{
    using namespace testing;
    assert(global_mock_android_hw == NULL && "Only one mock object per process is allowed");
    global_mock_android_hw = this;

    mock_alloc_device = std::make_shared<NiceMock<mtd::MockAllocDevice>>();
    mock_gralloc_module = std::make_shared<mtd::HardwareModuleStub>(mock_alloc_device->common);

    mock_hwc_device = std::make_shared<NiceMock<mtd::MockHWCComposerDevice1>>();
    mock_hwc_module = std::make_shared<mtd::HardwareModuleStub>(mock_hwc_device->common);

    ON_CALL(*this, hw_get_module(StrEq(GRALLOC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_gralloc_module.get()), Return(0)));
    ON_CALL(*this, hw_get_module(StrEq(HWC_HARDWARE_MODULE_ID),_))
        .WillByDefault(DoAll(SetArgPointee<1>(mock_hwc_module.get()), Return(0)));

    open_count.store(0);
}

bool mtd::HardwareAccessMock::open_count_matches_close()
{
    return (open_count == 0);
}

mtd::HardwareAccessMock::~HardwareAccessMock()
{
    global_mock_android_hw = NULL;
}

int hw_get_module(const char *id, const struct hw_module_t **module)
{
    if (!global_mock_android_hw)
    {
        ADD_FAILURE_AT(__FILE__,__LINE__); \
        return -1;
    }
    int rc =  global_mock_android_hw->hw_get_module(id, module);
    return rc;
}
