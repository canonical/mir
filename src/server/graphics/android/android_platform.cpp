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

#include "android_platform.h"
#include "android_graphic_buffer_allocator.h"
#include "android_hwc_factory.h"
#include "android_display_allocator.h"
#include "android_display_factory.h"
#include "default_framebuffer_factory.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/compositor/buffer_id.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir_protobuf.pb.h"

namespace mp=mir::protobuf;
namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;

mga::AndroidPlatform::AndroidPlatform(std::shared_ptr<mg::DisplayReport> const& display_report)
    : display_report(display_report)
{
}

std::shared_ptr<mc::GraphicBufferAllocator> mga::AndroidPlatform::create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& buffer_initializer)
{
    return std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer);
}

std::shared_ptr<mg::Display> mga::AndroidPlatform::create_display()
{
    auto hwc_factory = std::make_shared<mga::AndroidHWCFactory>();
    auto display_allocator = std::make_shared<mga::AndroidDisplayAllocator>();

    auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto buffer_allocator = std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer);
    auto fb_factory = std::make_shared<mga::DefaultFramebufferFactory>(buffer_allocator);
    auto display_factory = std::make_shared<mga::AndroidDisplayFactory>(display_allocator, hwc_factory, fb_factory, display_report);
    return display_factory->create_display();
}

std::shared_ptr<mg::PlatformIPCPackage> mga::AndroidPlatform::get_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>();
}

void mga::AndroidPlatform::fill_ipc_package(mp::Buffer* response, std::shared_ptr<mc::Buffer> const& buffer) const
{
    auto native_buffer = buffer->native_buffer_handle();
    auto buffer_handle = native_buffer->handle;

    int offset = 0;
    for(auto i=0; i<buffer_handle->numFds; i++)
    {
        response->add_fd(buffer_handle->data[offset++]);
    }    
    for(auto i=0; i<buffer_handle->numInts; i++)
    {
        response->add_data(buffer_handle->data[offset++]);
    }    

    response->set_stride(buffer->stride().as_uint32_t()); 
}
#if 0
std::shared_ptr<mc::BufferIPCPackage> mga::AndroidPlatform::create_buffer_ipc_package(
    std::shared_ptr<mc::Buffer> const& buffer) const
{
    auto ipc_package = std::make_shared<mc::BufferIPCPackage>();

    const native_handle_t *native_handle = buffer->native_buffer_handle()->handle;

    ipc_package->ipc_fds.resize(native_handle->numFds);
    int offset = 0;
    for(auto it=ipc_package->ipc_fds.begin(); it != ipc_package->ipc_fds.end(); it++)
    {
        *it = native_handle->data[offset++];
    }

    ipc_package->ipc_data.resize(native_handle->numInts);
    for(auto it=ipc_package->ipc_data.begin(); it != ipc_package->ipc_data.end(); it++)
    {
        *it = native_handle->data[offset++];
    }

    ipc_package->stride = buffer->stride().as_uint32_t();

    return ipc_package;
}
#endif
EGLNativeDisplayType mga::AndroidPlatform::shell_egl_display()
{
    // TODO: Implement
    return static_cast<EGLNativeDisplayType>(0);
}

std::shared_ptr<mg::Platform> mg::create_platform(std::shared_ptr<DisplayReport> const& display_report)
{
    return std::make_shared<mga::AndroidPlatform>(display_report);
}
