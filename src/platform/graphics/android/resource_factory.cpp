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
#include "buffer.h"
#include "resource_factory.h"
#include "fb_device.h"
#include "graphic_buffer_allocator.h"
#include "server_render_window.h"
#include "interpreter_cache.h"
#include "hwc11_device.h"
#include "hwc10_device.h"
#include "hwc_layerlist.h"
#include "hwc_vsync.h"
#include "android_display.h"

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

std::shared_ptr<hwc_composer_device_1> mga::ResourceFactory::create_hwc_native_device() const
{
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

    return std::shared_ptr<hwc_composer_device_1>(
        hwc_device_raw,
        [](hwc_composer_device_1* device) { device->common.close((hw_device_t*) device); });
}

std::shared_ptr<ANativeWindow> mga::ResourceFactory::create_native_window(
    std::shared_ptr<mga::FramebufferBundle> const& fb_bundle) const
{
    auto cache = std::make_shared<mga::InterpreterCache>();
    auto interpreter = std::make_shared<ServerRenderWindow>(fb_bundle, cache);
    return std::make_shared<MirNativeWindow>(interpreter);
}

std::shared_ptr<mga::DisplayDevice> mga::ResourceFactory::create_fb_device(
    std::shared_ptr<framebuffer_device_t> const& fb_native_device) const
{
    return std::make_shared<mga::FBDevice>(fb_native_device);
}

std::shared_ptr<mga::DisplayDevice> mga::ResourceFactory::create_hwc11_device(
    std::shared_ptr<hwc_composer_device_1> const& hwc_native_device) const
{
    auto syncer = std::make_shared<mga::HWCVsync>();
    return std::make_shared<mga::HWC11Device>(hwc_native_device, syncer);
}

std::shared_ptr<mga::DisplayDevice> mga::ResourceFactory::create_hwc10_device(
    std::shared_ptr<hwc_composer_device_1> const& hwc_native_device,
    std::shared_ptr<framebuffer_device_t> const& fb_native_device) const
{
    auto syncer = std::make_shared<mga::HWCVsync>();
    return std::make_shared<mga::HWC10Device>(hwc_native_device, fb_native_device, syncer);
}
