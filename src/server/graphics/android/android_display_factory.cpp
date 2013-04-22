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

#include "android_display_factory.h"
#include "hwc_factory.h"
#include "framebuffer_factory.h"
#include "display_allocator.h"
#include "android_display.h"
#include "hwc_display.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

mga::AndroidDisplayFactory::AndroidDisplayFactory(std::shared_ptr<DisplayAllocator> const& display_factory,
                                                  std::shared_ptr<HWCFactory> const& hwc_factory,
                                                  std::shared_ptr<FramebufferFactory> const& fb_factory)
    : display_factory(display_factory),
      hwc_factory(hwc_factory),
      fb_factory(fb_factory),
      fb_dev(fb_factory->create_fb_device())
{
    const hw_module_t *hw_module;
    int rc = hw_get_module(HWC_HARDWARE_MODULE_ID, &hw_module);    
    if ((rc != 0) || (hw_module == nullptr))
    {
        return;
    }

    try
    {
        setup_hwc_dev(hw_module);
    } catch (std::runtime_error &e)
    {
        /* TODO: log error. this is nonfatal, we'll just create the backup display */
    }
}

void mga::AndroidDisplayFactory::setup_hwc_dev(const hw_module_t* module)
{
    if ((!module->methods) || !(module->methods->open))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("display factory cannot create hwc display"));
    }

    hwc_composer_device_1* hwc_device_raw = nullptr;
    int rc = module->methods->open(module, HWC_HARDWARE_COMPOSER, reinterpret_cast<hw_device_t**>(&hwc_device_raw));

    if ((rc != 0) || (hwc_device_raw == nullptr))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("display hwc module unusable"));
    }

    hwc_dev = std::shared_ptr<hwc_composer_device_1>(hwc_device_raw,
                  [](hwc_composer_device_1* device)
                  {
                     device->common.close((hw_device_t*) device);
                  });
}

std::shared_ptr<mg::Display> mga::AndroidDisplayFactory::create_display() const
{
    //TODO: if hwc display creation fails, we could try the gpu display
    if (hwc_dev && (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_1))
    {
        auto hwc_device = hwc_factory->create_hwc_1_1(hwc_dev, fb_dev);
        auto fb_native_win = fb_factory->create_fb_native_window(hwc_device);
        return display_factory->create_hwc_display(hwc_device, fb_native_win);
    }
    else if (hwc_dev && (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_0))
    {
        auto hwc_device = hwc_factory->create_hwc_1_0(hwc_dev, fb_dev);
        auto fb_native_win = fb_factory->create_fb_native_window(hwc_device);
        return display_factory->create_hwc_display(hwc_device, fb_native_win);
    }
    else
    {
        auto fb_native_win = fb_factory->create_fb_native_window(fb_dev);
        return display_factory->create_gpu_display(fb_native_win);
    }
}
