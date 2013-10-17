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

mga::ResourceFactory::ResourceFactory(std::shared_ptr<GraphicBufferAllocator> const& buffer_allocator)
    : buffer_allocator(buffer_allocator)
{
}

std::vector<std::shared_ptr<mg::Buffer>> mga::ResourceFactory::create_buffers(
    std::shared_ptr<DisplaySupportProvider> const& info_provider) const
{
    auto size = info_provider->display_size();
    auto pf = info_provider->display_format();
    auto num_framebuffers = info_provider->number_of_framebuffers_available();
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    for (auto i = 0u; i < num_framebuffers; ++i)
    {
        buffers.push_back(buffer_allocator->alloc_buffer_platform(size, pf, mga::BufferUsage::use_framebuffer_gles));
    }
    return buffers;
}

std::shared_ptr<mga::FBSwapper> mga::ResourceFactory::create_swapper(
    std::vector<std::shared_ptr<mg::Buffer>> const& buffers) const
{
    return std::make_shared<mga::FBSimpleSwapper>(buffers);
}

std::shared_ptr<mga::DisplaySupportProvider> mga::ResourceFactory::create_fb_native_device() const
{
    hw_module_t const* module;
    framebuffer_device_t* fbdev_raw;

    auto rc = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    if ((rc != 0) || (module == nullptr) || (framebuffer_open(module, &fbdev_raw) != 0) )
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("display factory cannot create fb display")); 
    }

    auto fb_dev = std::shared_ptr<framebuffer_device_t>(fbdev_raw,
                      [](struct framebuffer_device_t* fbdevice)
                      {
                         fbdevice->common.close((hw_device_t*) fbdevice);
                      });

    return std::make_shared<mga::FBDevice>(fb_dev);
}

std::shared_ptr<hwc_composer_device_1> mga::ResourceFactory::create_hwc_native_device() const
{
    const hw_module_t *module;
    int rc = hw_get_module(HWC_HARDWARE_MODULE_ID, &module);
    if ((rc != 0) || (module == nullptr))
    {
        //SHOULD THROW
        // this is nonfatal, we'll just create the backup display
        return {};
    }

    if ((!module->methods) || !(module->methods->open))
    {
        // TODO: log "display factory cannot create hwc display".
        // this is nonfatal, we'll just create the backup display
        return {};
    }

    hwc_composer_device_1* hwc_device_raw = nullptr;
    rc = module->methods->open(module, HWC_HARDWARE_COMPOSER, reinterpret_cast<hw_device_t**>(&hwc_device_raw));

    if ((rc != 0) || (hwc_device_raw == nullptr))
    {
        // TODO: log "display hwc module unusable".
        // this is nonfatal, we'll just create the backup display
        return {};
    }

    return std::shared_ptr<hwc_composer_device_1>(
        hwc_device_raw,
        [](hwc_composer_device_1* device) { device->common.close((hw_device_t*) device); });
}
 
//create hwc native
std::shared_ptr<mga::DisplaySupportProvider> mga::ResourceFactory::create_hwc_1_1(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<mga::DisplaySupportProvider> const& fb_device) const
{
    auto layer_list = std::make_shared<mga::LayerList>();
    auto syncer = std::make_shared<mga::HWCVsync>();
    return std::make_shared<mga::HWC11Device>(hwc_device, layer_list, fb_device, syncer);
}

std::shared_ptr<mga::DisplaySupportProvider> mga::ResourceFactory::create_hwc_1_0(
    std::shared_ptr<hwc_composer_device_1> const& hwc_device,
    std::shared_ptr<mga::DisplaySupportProvider> const& fb_device) const
{
    auto syncer = std::make_shared<mga::HWCVsync>();
    return std::make_shared<mga::HWC10Device>(hwc_device, fb_device, syncer);
}


//really, this function shouldnt be here
std::shared_ptr<mg::Display> mga::ResourceFactory::create_display(
    std::shared_ptr<DisplaySupportProvider> fb_dev,
    std::shared_ptr<hwc_composer_device_1> hwc_dev,
    std::shared_ptr<DisplayReport> const display_report,
{
    std::shared_ptr<mga::DisplayInfo> info;

    if (hwc_dev && (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_1))
    {
        info = create_hwc_1_1_info(hwc_dev);
    }
    else if (hwc_dev && (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_0))
    {
        info = create_hwc_1_0_info(hwc_dev, fb_dev);
    }
    else
    {
        info = create_fb_info(fb_dev);
    }

    auto buffers = create_buffers(info);
    auto swapper = create_swapper(buffers);

    std::shared_ptr<mga::DisplayCommand> command;
    //TODO: if hwc display creation fails, we could try the gpu display
    if (hwc_dev && (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_1))
    {
        command = create_hwc_1_1(hwc_dev);
        display_report->report_hwc_composition_in_use(1,1);
    }
    else if (hwc_dev && (hwc_dev->common.version == HWC_DEVICE_API_VERSION_1_0))
    {
        command = create_hwc_1_0(hwc_dev, fb_dev);
        display_report->report_hwc_composition_in_use(1,0);
    }
    else
    {
        command = create_fb_command();
        display_report->report_gpu_composition_in_use();
    }

    auto cache = std::make_shared<mga::InterpreterCache>();
    auto interpreter = std::make_shared<mga::ServerRenderWindow>(swapper, support_provider, cache);
    auto native_window = std::make_shared<mga::MirNativeWindow>(interpreter);
    auto db_factory = std::make_shared<mga::DisplayBufferFactory>();
    return std::make_shared<AndroidDisplay>(native_window, db_factory, info, command, report);
}

