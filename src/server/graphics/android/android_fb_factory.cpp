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

#include "android_fb_factory.h"
#include "android_framebuffer_window.h"
#include "android_display.h"
#include "hwc_display.h"
#include "hwc_factory.h"
#include "hwc11_device.h"

#include <boost/throw_exception.hpp>
#include <ui/FramebufferNativeWindow.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

mga::AndroidFBFactory::AndroidFBFactory(std::shared_ptr<HWCFactory> const& hwc_factory)
    : hwc_factory(hwc_factory),
      is_hwc_capable(false)
{
    const hw_module_t *hw_module;
    int rc = hw_get_module(HWC_HARDWARE_MODULE_ID, &hw_module);
    if ((rc != 0) || (hw_module == nullptr))
    {
        return;
    }

    hwc_composer_device_1* hwc_device_raw;
    rc = hw_module->methods->open(hw_module, HWC_HARDWARE_COMPOSER, (hw_device_t**) &hwc_device_raw);
    if ((rc != 0) || (hwc_device_raw == nullptr))
    {
        return;
    }

    auto hwc_dev = std::shared_ptr<hwc_composer_device_1>( hwc_device_raw,
                            [](hwc_composer_device_1* device)
                            {
                                device->common.close((hw_device_t*) device);
                            });
    if (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_1)
    {
        hwc_device = hwc_factory->create_hwc_1_1();
        is_hwc_capable = true;
    }
}

bool mga::AndroidFBFactory::can_create_hwc_display() const
{
    return is_hwc_capable;
}

std::shared_ptr<mg::Display> mga::AndroidFBFactory::create_hwc_display() const
{
    if(!is_hwc_capable)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("factory cannot create hwc display"));
    }

    return std::shared_ptr<mg::Display>();
#if 0
    auto android_window = std::make_shared< ::android::FramebufferNativeWindow>();
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);
    auto hwc = std::make_shared<mga::HWC11Device>(hwc_device);
    return std::make_shared<mga::HWCDisplay>(window, hwc);
#endif
}

/* note: gralloc seems to choke when this is opened/closed more than once per process. must investigate drivers further */
std::shared_ptr<mg::Display> mga::AndroidFBFactory::create_gpu_display() const
{
    auto android_window = std::make_shared< ::android::FramebufferNativeWindow>();
    auto window = std::make_shared<mga::AndroidFramebufferWindow> (android_window);

    return std::make_shared<mga::AndroidDisplay>(window);
}
