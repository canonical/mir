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
#include "internal_client.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/buffer_initializer.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_ipc_packer.h"
#include "mir/options/option.h"


#include "mir/graphics/native_platform.h"
#include "mir/graphics/null_display_report.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mf=mir::frontend;
namespace mo = mir::options;

mga::AndroidPlatform::AndroidPlatform(std::shared_ptr<mg::DisplayReport> const& display_report)
    : display_report(display_report)
{
}

std::shared_ptr<mg::GraphicBufferAllocator> mga::AndroidPlatform::create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& buffer_initializer)
{
    return std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer);
}

std::shared_ptr<mga::GraphicBufferAllocator> mga::AndroidPlatform::create_mga_buffer_allocator(
    const std::shared_ptr<mg::BufferInitializer>& buffer_initializer)
{
    return std::make_shared<mga::AndroidGraphicBufferAllocator>(buffer_initializer);
}

std::shared_ptr<mga::FramebufferFactory> mga::AndroidPlatform::create_frame_buffer_factory(
    const std::shared_ptr<mga::GraphicBufferAllocator>& buffer_allocator)
{
    return std::make_shared<mga::DefaultFramebufferFactory>(buffer_allocator);
}

std::shared_ptr<mg::Display> mga::AndroidPlatform::create_display(
    std::shared_ptr<graphics::DisplayConfigurationPolicy> const&)
{
    auto hwc_factory = std::make_shared<mga::AndroidHWCFactory>();
    auto display_allocator = std::make_shared<mga::AndroidDisplayAllocator>();

    auto buffer_initializer = std::make_shared<mg::NullBufferInitializer>();
    auto buffer_allocator = create_mga_buffer_allocator(buffer_initializer);
    auto fb_factory = create_frame_buffer_factory(buffer_allocator);
    mga::AndroidDisplayFactory display_factory(display_allocator, hwc_factory, fb_factory, display_report);
    return display_factory.create_display();
}

std::shared_ptr<mg::PlatformIPCPackage> mga::AndroidPlatform::get_ipc_package()
{
    return std::make_shared<mg::PlatformIPCPackage>();
}

void mga::AndroidPlatform::fill_ipc_package(std::shared_ptr<BufferIPCPacker> const& packer,
                                            std::shared_ptr<mg::Buffer> const& buffer) const
{
    auto native_buffer = buffer->native_buffer_handle();
    auto buffer_handle = native_buffer->handle;

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

std::shared_ptr<mg::InternalClient> mga::AndroidPlatform::create_internal_client()
{
    return std::make_shared<mga::InternalClient>();
}

extern "C" std::shared_ptr<mg::Platform> mg::create_platform(std::shared_ptr<mo::Option> const& /*options*/, std::shared_ptr<DisplayReport> const& display_report)
{
    return std::make_shared<mga::AndroidPlatform>(display_report);
}

namespace
{
// It may well turn out that NativePlatform == Platform - but this keeps them separate for now
struct NativeAndroidPlatform : mg::NativePlatform
{

    NativeAndroidPlatform()
        : underlying(std::make_shared<mga::AndroidPlatform>(std::make_shared<mg::NullDisplayReport>()))
    {

    }
    std::shared_ptr<mg::Platform> underlying;

    void initialize(int /*data_items*/, int const* /*data*/, int /*fd_items*/, int const* /*fd*/) override
    {
    }

    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator(
        std::shared_ptr<mg::BufferInitializer> const& init) override
    {
        return underlying->create_buffer_allocator(init);
    }

    std::shared_ptr<mg::PlatformIPCPackage> get_ipc_package() override
    {
        return underlying->get_ipc_package();
    }

    std::shared_ptr<mg::InternalClient> create_internal_client() override
    {
        return underlying->create_internal_client();
    }

    void fill_ipc_package(std::shared_ptr<mg::BufferIPCPacker> const& packer,
            std::shared_ptr<mg::Buffer> const& buf) const override
    {
       underlying->fill_ipc_package(packer, buf);
    }
};
}

extern "C" std::shared_ptr<mg::NativePlatform> create_native_platform()
{
    return std::make_shared<::NativeAndroidPlatform>();
}
