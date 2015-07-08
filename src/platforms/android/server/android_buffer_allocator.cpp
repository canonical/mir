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
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/android_native_buffer.h"
#include "android_graphic_buffer_allocator.h"
#include "android_alloc_adaptor.h"
#include "buffer.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mg  = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

namespace
{
struct AllocDevDeleter
{
    void operator()(alloc_device_t* t)
    {
        /* android takes care of delete for us */
        t->common.close((hw_device_t*)t);
    }
};
}

mga::AndroidGraphicBufferAllocator::AndroidGraphicBufferAllocator(std::shared_ptr<DeviceQuirks> const& quirks)
    : egl_extensions(std::make_shared<mg::EGLExtensions>())
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

    AllocDevDeleter del;
    std::shared_ptr<struct alloc_device_t> alloc_dev_ptr(alloc_dev, del);
    alloc_device = std::shared_ptr<mga::GraphicAllocAdaptor>(new AndroidAllocAdaptor(alloc_dev_ptr, quirks));
}

std::shared_ptr<mg::Buffer> mga::AndroidGraphicBufferAllocator::alloc_buffer(
    mg::BufferProperties const& buffer_properties)
{
    auto usage = convert_from_compositor_usage(buffer_properties.usage);
    return alloc_buffer_platform(buffer_properties.size, buffer_properties.format, usage);
}

std::unique_ptr<mg::Buffer> mga::AndroidGraphicBufferAllocator::reconstruct_from(
    ANativeWindowBuffer* anwb, MirPixelFormat)
{
    if (!anwb->common.incRef || !anwb->common.decRef)
        BOOST_THROW_EXCEPTION(std::runtime_error("Could not claim a reference (incRef or decRef was null)"));
    std::shared_ptr<ANativeWindowBuffer> native_window_buffer(anwb,
        [](ANativeWindowBuffer* buffer){ buffer->common.decRef(&buffer->common); });
    anwb->common.incRef(&anwb->common);

    auto native_handle = std::make_shared<mga::AndroidNativeBuffer>(
        native_window_buffer,
        //TODO: we should have an android platform function for accessing the fence.
        std::make_shared<mga::SyncFence>(std::make_shared<mga::RealSyncFileOps>(), mir::Fd()),
        mga::BufferAccess::read);
    return std::make_unique<Buffer>(
        reinterpret_cast<gralloc_module_t const*>(hw_module), native_handle, egl_extensions);
}

std::shared_ptr<mg::Buffer> mga::AndroidGraphicBufferAllocator::alloc_buffer_platform(
    geom::Size sz, MirPixelFormat pf, mga::BufferUsage use)
{
    auto native_handle = alloc_device->alloc_buffer(sz, pf, use);
    auto buffer = std::make_shared<Buffer>(reinterpret_cast<gralloc_module_t const*>(hw_module), native_handle, egl_extensions);

    return buffer;
}

std::vector<MirPixelFormat> mga::AndroidGraphicBufferAllocator::supported_pixel_formats()
{
    static std::vector<MirPixelFormat> const pixel_formats{
        mir_pixel_format_abgr_8888,
        mir_pixel_format_xbgr_8888,
        mir_pixel_format_rgb_888,
        mir_pixel_format_rgb_565
    };

    return pixel_formats;
}

mga::BufferUsage mga::AndroidGraphicBufferAllocator::convert_from_compositor_usage(mg::BufferUsage usage)
{
    switch (usage)
    {
        case mg::BufferUsage::software:
            return mga::BufferUsage::use_software;
        case mg::BufferUsage::hardware:
        default:
            return mga::BufferUsage::use_hardware;
    }
}
