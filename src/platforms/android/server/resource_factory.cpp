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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/mir_native_window.h"
#include "mir/graphics/android/sync_fence.h"
#include "buffer.h"
#include "resource_factory.h"
#include "fb_device.h"
#include "graphic_buffer_allocator.h"
#include "server_render_window.h"
#include "hwc_device.h"
#include "hwc_fb_device.h"
#include "hwc_layerlist.h"
#include "display.h"
#include "real_hwc_wrapper.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <vector>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;

std::shared_ptr<framebuffer_device_t> mga::ResourceFactory::create_fb_native_device() const
{
    hw_module_t const* module;
    framebuffer_device_t* fbdev_raw;

    auto rc = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if ((rc != 0) || (module == nullptr) || (framebuffer_open(module, &fbdev_raw) != 0) )
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("display factory cannot create fb display"));
    }

    return std::shared_ptr<framebuffer_device_t>(fbdev_raw,
                      [](struct framebuffer_device_t* fbdevice)
                      {
                         fbdevice->common.close((hw_device_t*) fbdevice);
                      });
}

std::tuple<std::shared_ptr<mga::HwcWrapper>, mga::HwcVersion>
mga::ResourceFactory::create_hwc_wrapper(std::shared_ptr<mga::HwcReport> const& hwc_report) const
{
    //TODO: could probably be collapsed further into HwcWrapper's constructor
    hwc_composer_device_1* hwc_device_raw = nullptr;
    hw_module_t const *module;
    int rc = hw_get_module(HWC_HARDWARE_MODULE_ID, &module);
    if ((rc != 0) || (module == nullptr) ||
       (!module->methods) || !(module->methods->open) ||
       module->methods->open(module, HWC_HARDWARE_COMPOSER, reinterpret_cast<hw_device_t**>(&hwc_device_raw)) ||
       (hwc_device_raw == nullptr))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error opening hwc hal"));
    }

    auto hwc_native = std::shared_ptr<hwc_composer_device_1>(hwc_device_raw,
            [](hwc_composer_device_1* device) { device->common.close((hw_device_t*) device); });

    auto version = mga::HwcVersion::hwc10;
    switch(hwc_native->common.version)
    {
        case HWC_DEVICE_API_VERSION_1_0: version = mga::HwcVersion::hwc10; break;
        case HWC_DEVICE_API_VERSION_1_1: version = mga::HwcVersion::hwc11; break;
        case HWC_DEVICE_API_VERSION_1_2: version = mga::HwcVersion::hwc12; break;
        case HWC_DEVICE_API_VERSION_1_3: version = mga::HwcVersion::hwc13; break;
        case HWC_DEVICE_API_VERSION_1_4: version = mga::HwcVersion::hwc14; break;
        default: version = mga::HwcVersion::unknown; break;
    }
    
    return std::make_tuple(
        std::make_shared<mga::RealHwcWrapper>(hwc_native, hwc_report),
        version);
}
