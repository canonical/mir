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

mga::ResourceFactory::ResourceFactory(
    std::shared_ptr<GraphicBufferAllocator> const& buffer_allocator)
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

std::shared_ptr<mga::DisplaySupportProvider> mga::ResourceFactory::create_fb_device() const
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

std::shared_ptr<mg::Display> mga::ResourceFactory::create_display(
    std::shared_ptr<mga::DisplaySupportProvider> const& support_provider,
    std::shared_ptr<mg::DisplayReport> const& report) const
{
    auto buffers = create_buffers(support_provider);
    auto swapper = create_swapper(buffers);
    auto cache = std::make_shared<mga::InterpreterCache>();
    auto interpreter = std::make_shared<mga::ServerRenderWindow>(swapper, support_provider, cache);
    auto native_win = std::make_shared<mga::MirNativeWindow>(interpreter);
    auto window = std::make_shared<mga::AndroidFramebufferWindow>(native_win);
    auto db_factory = std::make_shared<mga::DisplayBufferFactory>();
    return std::make_shared<AndroidDisplay>(window, db_factory, support_provider, report);
}
