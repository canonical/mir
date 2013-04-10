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

#include "android_hwc_factory.h"
#include "hwc11_device.h"
#include "hwc_layerlist.h"
#include "default_fb_device.h"

namespace mga=mir::graphics::android;

std::shared_ptr<mga::HWCDevice> mga::AndroidHWCFactory::create_hwc_1_1(std::shared_ptr<hwc_composer_device_1> const& hwc_device) const
{
    auto layer_list = std::make_shared<mga::HWCLayerList>();

    hw_module_t const* module;
    framebuffer_device_t* fbdev_raw;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module) == 0) {
        framebuffer_open(module, &fbdev_raw);
    }
    std::shared_ptr<framebuffer_device_t> fbdev(fbdev_raw, [](framebuffer_device_t*){});
    auto fb = std::make_shared<mga::DefaultFBDevice>(fbdev);
    return std::make_shared<mga::HWC11Device>(hwc_device, layer_list, fb);
}
