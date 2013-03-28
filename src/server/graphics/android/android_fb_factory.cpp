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
#include "hwc11_device.h"

#include <ui/FramebufferNativeWindow.h>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;

mga::AndroidFBFactory::AndroidFBFactory(std::shared_ptr<HWCFactory> const& /*hwc_factory*/)
    : is_hwc_capable(false)
{
    const hw_module_t *hw_module;
    int rc = hw_get_module(HWC_HARDWARE_MODULE_ID, &hw_module);
    if (rc != 0)
    {
        return;
    }

}

bool mga::AndroidFBFactory::can_create_hwc_display() const
{
    return is_hwc_capable;
}

std::shared_ptr<mg::Display> mga::AndroidFBFactory::create_hwc_display() const
{
    return create_gpu_display();
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
