/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/mir_native_window.h"
#include "buffer.h"
#include "resource_factory.h"
#include "fb_device.h"
#include "fb_simple_swapper.h"
#include "graphic_buffer_allocator.h"
#include "server_render_window.h"
#include "interpreter_cache.h"
#include "hwc11_device.h"
#include "hwc10_device.h"
#include "hwc_layerlist.h"
#include "hwc_vsync.h"
#include "android_display.h"
#include "display_buffer_factory.h"

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
 
std::shared_ptr<mga::FBSwapper> mga::ResourceFactory::create_fb_buffers(
    std::shared_ptr<mga::DisplayDevice> const& device,
    std::shared_ptr<mga::GraphicBufferAllocator> const& buffer_allocator) const
{
    auto size = device->display_size();
    auto pf = device->display_format();
    auto num_framebuffers = device->number_of_framebuffers_available();
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    for (auto i = 0u; i < num_framebuffers; ++i)
    {
        buffers.push_back(buffer_allocator->alloc_buffer_platform(size, pf, mga::BufferUsage::use_framebuffer_gles));
    }
    return std::make_shared<mga::FBSimpleSwapper>(buffers);
}

std::shared_ptr<mga::DisplayDevice> mga::ResourceFactory::create_fb_device(
    std::shared_ptr<framebuffer_device_t> const& fb_native_device) const
{
    return std::make_shared<mga::FBDevice>(fb_native_device);
}

std::shared_ptr<mga::DisplayDevice> mga::ResourceFactory::create_hwc11_device(
    std::shared_ptr<hwc_composer_device_1> const& hwc_native_device) const
{
    auto layer_list = std::make_shared<mga::LayerList>();
    auto syncer = std::make_shared<mga::HWCVsync>();
    return std::make_shared<mga::HWC11Device>(hwc_native_device, layer_list, nullptr, syncer);
}

std::shared_ptr<mga::DisplayDevice> mga::ResourceFactory::create_hwc10_device(
    std::shared_ptr<hwc_composer_device_1> const& hwc_native_device,
    std::shared_ptr<framebuffer_device_t> const& fb_native_device) const
{
    auto syncer = std::make_shared<mga::HWCVsync>();
    auto fb_device = create_fb_device(fb_native_device);
    return std::make_shared<mga::HWC10Device>(hwc_native_device, fb_device, syncer);
}

std::shared_ptr<mg::Display> mga::ResourceFactory::create_display(
    std::shared_ptr<FBSwapper> const& swapper,
    std::shared_ptr<DisplayDevice> const& device,
    std::shared_ptr<graphics::DisplayReport> const& report) const
{
    auto cache = std::make_shared<mga::InterpreterCache>();
    auto db_factory = std::make_shared<mga::DisplayBufferFactory>();

    auto interpreter = std::make_shared<mga::ServerRenderWindow>(swapper, device, cache);
    auto native_window = std::make_shared<mga::MirNativeWindow>(interpreter);
    return std::make_shared<AndroidDisplay>(native_window, db_factory, device, report);
}
