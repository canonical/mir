/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/graphics/platform.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/buffer_properties.h"
#include "cmdstream_sync_factory.h"
#include "sync_fence.h"
#include "android_native_buffer.h"
#include "graphic_buffer_allocator.h"
#include "gralloc_module.h"
#include "buffer.h"
#include "device_quirks.h"
#include "egl_sync_fence.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mg  = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

namespace
{

void alloc_dev_deleter(alloc_device_t* t)
{
    /* android takes care of delete for us */
    t->common.close((hw_device_t*)t);
}

void null_alloc_dev_deleter(alloc_device_t*)
{
}

}

mga::GraphicBufferAllocator::GraphicBufferAllocator(
    std::shared_ptr<CommandStreamSyncFactory> const& cmdstream_sync_factory,
    std::shared_ptr<DeviceQuirks> const& quirks)
    : egl_extensions(std::make_shared<mg::EGLExtensions>()),
    cmdstream_sync_factory(cmdstream_sync_factory)
{
    int err;

    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
    if (err < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not open hardware module"));

    struct alloc_device_t* alloc_dev;
    err = hw_module->methods->open(hw_module, GRALLOC_HARDWARE_GPU0, (struct hw_device_t**) &alloc_dev);
    if (err < 0)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not open hardware module"));

    /* note for future use: at this point, the hardware module should be filled with vendor information
       that we can determine different courses of action based upon */

    std::shared_ptr<struct alloc_device_t> alloc_dev_ptr(
        alloc_dev,
        quirks->gralloc_cannot_be_closed_safely() ? null_alloc_dev_deleter : alloc_dev_deleter);
    alloc_device = std::make_shared<mga::GrallocModule>(
        alloc_dev_ptr, cmdstream_sync_factory, quirks);
}

std::shared_ptr<mg::Buffer> mga::GraphicBufferAllocator::alloc_buffer(
    mg::BufferProperties const& properties)
{
    return std::make_shared<Buffer>(
        reinterpret_cast<gralloc_module_t const*>(hw_module),
        alloc_device->alloc_buffer(properties.size, properties.format, properties.usage),
        egl_extensions);
}

std::shared_ptr<mg::Buffer> mga::GraphicBufferAllocator::alloc_framebuffer(
    geometry::Size sz, MirPixelFormat pf)
{
    return std::make_shared<Buffer>(
        reinterpret_cast<gralloc_module_t const*>(hw_module),
        alloc_device->alloc_framebuffer(sz, pf),
        egl_extensions);
}

std::unique_ptr<mg::Buffer> mga::GraphicBufferAllocator::reconstruct_from(
    ANativeWindowBuffer* anwb, MirPixelFormat)
{
    if (!anwb->common.incRef || !anwb->common.decRef)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not claim a reference (incRef or decRef was null)"));
    std::shared_ptr<ANativeWindowBuffer> native_window_buffer(anwb,
        [](ANativeWindowBuffer* buffer){ buffer->common.decRef(&buffer->common); });
    anwb->common.incRef(&anwb->common);

    //TODO: we should have an android platform function for accessing the fence.
    auto native_handle = std::make_shared<mga::AndroidNativeBuffer>(
        native_window_buffer,
        cmdstream_sync_factory->create_command_stream_sync(),
        std::make_shared<mga::SyncFence>(std::make_shared<mga::RealSyncFileOps>(), mir::Fd()),
        mga::BufferAccess::read);
    return std::make_unique<Buffer>(
        reinterpret_cast<gralloc_module_t const*>(hw_module), native_handle, egl_extensions);
}

std::vector<MirPixelFormat> mga::GraphicBufferAllocator::supported_pixel_formats()
{
    static std::vector<MirPixelFormat> const pixel_formats{
        mir_pixel_format_abgr_8888,
        mir_pixel_format_xbgr_8888,
        mir_pixel_format_rgb_888,
        mir_pixel_format_rgb_565
    };

    return pixel_formats;
}
