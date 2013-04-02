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

#include "android_display_selector.h"
#include "android_fb_factory.h"

#include <hardware/hwcomposer.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

mga::AndroidDisplaySelector::AndroidDisplaySelector(std::shared_ptr<mga::FBFactory> const& factory)
    : fb_factory(factory)
{
    const hw_module_t *hw_module;
    int rc = hw_get_module(HWC_HARDWARE_MODULE_ID, &hw_module);

    if (rc != 0)
    {
        /* could not find hw module. use GL fallback */
        allocate_primary_fb = std::bind(&FBFactory::create_gpu_display, fb_factory);
        return;
    }

    hw_device_t* hwc_device;
    rc = hw_module->methods->open(hw_module, HWC_HARDWARE_COMPOSER, &hwc_device);

    if ((hwc_device != nullptr) &&
        (rc == 0 ) &&
        (hwc_device->version == HWC_DEVICE_API_VERSION_1_1))
    {
        allocate_primary_fb = std::bind(&FBFactory::create_hwc1_1_gpu_display, fb_factory);
    }
    else
    {
        allocate_primary_fb = std::bind(&FBFactory::create_gpu_display, fb_factory);
    }
}

std::shared_ptr<mg::Display> mga::AndroidDisplaySelector::primary_display()
{
    return allocate_primary_fb();
}
