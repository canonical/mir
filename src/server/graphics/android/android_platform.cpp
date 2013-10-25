/*
 * Copyright © 2012 Canonical Ltd.
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
 *   Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/android/sync_fence.h"
#include "android_platform.h"
#include "android_graphic_buffer_allocator.h"
#include "resource_factory.h"
#include "internal_client.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/android/native_buffer.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_ipc_packer.h"
#include "mir/options/option.h"

#include "mir/graphics/null_display_report.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mf=mir::frontend;
namespace mo = mir::options;

mga::AndroidPlatform::AndroidPlatform(
    std::shared_ptr<mga::DisplayResourceFactory> const& display_resource_factory,
    std::shared_ptr<mg::DisplayReport> const& display_report)
    : display_resource_factory(display_resource_factory),
      display_report(display_report)
{
}

std::shared_ptr<mg::GraphicBufferAllocator> mga::AndroidPlatform::create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& buffer_initializer)
{
    return std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer);
}

std::shared_ptr<mga::GraphicBufferAllocator> mga::AndroidPlatform::create_mga_buffer_allocator(
    std::shared_ptr<mg::BufferInitializer> const& buffer_initializer)
{
    return std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer);
}

std::shared_ptr<mg::Display> mga::AndroidPlatform::create_fb_backup_display()
{
    auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto buffer_allocator = create_mga_buffer_allocator(buffer_initializer);
    auto fb_native = display_resource_factory->create_fb_native_device();
    auto device = display_resource_factory->create_fb_device(fb_native);
    auto fb_swapper = display_resource_factory->create_fb_buffers(device, buffer_allocator);
    display_report->report_gpu_composition_in_use();
    return display_resource_factory->create_display(fb_swapper, device, display_report);
}

std::shared_ptr<mg::Display> mga::AndroidPlatform::create_display(
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const&)
{
    std::shared_ptr<hwc_composer_device_1> hwc_native;
    try
    {
        hwc_native = display_resource_factory->create_hwc_native_device();
    } catch (...)
    {
        return create_fb_backup_display();
    }

    //HWC 1.2 not supported yet. make an attempt to use backup display
    if (hwc_native->common.version == HWC_DEVICE_API_VERSION_1_2)
    {
        return create_fb_backup_display();
    }

    //we have a supported hwc, create it
    std::shared_ptr<mga::DisplayDevice> device; 
    if (hwc_native->common.version == HWC_DEVICE_API_VERSION_1_1)
    {
        device = display_resource_factory->create_hwc11_device(hwc_native);
        display_report->report_hwc_composition_in_use(1,1);
    }
    else if (hwc_native->common.version == HWC_DEVICE_API_VERSION_1_0)
    {
        auto fb_native = display_resource_factory->create_fb_native_device();
        device = display_resource_factory->create_hwc10_device(hwc_native, fb_native);
        display_report->report_hwc_composition_in_use(1,0);
    }

    auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto buffer_allocator = create_mga_buffer_allocator(buffer_initializer);
    auto fb_swapper = display_resource_factory->create_fb_buffers(device, buffer_allocator);
    return display_resource_factory->create_display(fb_swapper, device, display_report);
}

std::shared_ptr<mg::PlatformIPCPackage> mga::AndroidPlatform::get_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>();
}

void mga::AndroidPlatform::fill_ipc_package(std::shared_ptr<BufferIPCPacker> const& packer,
                                            std::shared_ptr<mg::Buffer> const& buffer) const
{
    auto native_buffer = buffer->native_buffer_handle();
    auto buffer_handle = native_buffer->handle();

    int offset = 0;
    
    for(auto i=0; i<buffer_handle->numFds; i++)
    {
        packer->pack_fd(buffer_handle->data[offset++]);
    }
    for(auto i=0; i<buffer_handle->numInts; i++)
    {
        packer->pack_data(buffer_handle->data[offset++]);
    }

    packer->pack_stride(buffer->stride());
}

void mga::AndroidPlatform::initialize(std::shared_ptr<NestedContext> const&)
{
}


std::shared_ptr<mg::InternalClient> mga::AndroidPlatform::create_internal_client()
{
    return std::make_shared<mga::InternalClient>();
}

extern "C" std::shared_ptr<mg::Platform> mg::create_platform(std::shared_ptr<mo::Option> const& /*options*/, std::shared_ptr<DisplayReport> const& display_report)
{
    auto display_resource_factory = std::make_shared<mga::ResourceFactory>();
    return std::make_shared<mga::AndroidPlatform>(display_resource_factory, display_report);
}

extern "C" std::shared_ptr<mg::NativePlatform> create_native_platform(std::shared_ptr<mg::DisplayReport> const& display_report)
{
    auto display_resource_factory = std::make_shared<mga::ResourceFactory>();
    return std::make_shared<mga::AndroidPlatform>(display_resource_factory, display_report);
}
