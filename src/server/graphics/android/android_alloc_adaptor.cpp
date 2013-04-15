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
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "android_alloc_adaptor.h"
#include "android_buffer_handle_default.h"

#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

namespace
{
struct AndroidBufferHandleDefaultDeleter
{
    AndroidBufferHandleDefaultDeleter(std::shared_ptr<alloc_device_t> alloc_dev)
        : alloc_device(alloc_dev)
    {}

    void operator()(mga::AndroidBufferHandleDefault* t)
    {
        ANativeWindowBuffer *anw_buffer = (ANativeWindowBuffer*) t->get_egl_client_buffer();
        alloc_device->free(alloc_device.get(), anw_buffer->handle);
        delete t;
    }
private:
    std::shared_ptr<alloc_device_t> alloc_device;
};

struct AndroidBufferHandleEmptyDeleter
{
    void operator()(mga::AndroidBufferHandle*)
    {
    }
};

static void incRef(android_native_base_t*)
{
}
}

mga::AndroidAllocAdaptor::AndroidAllocAdaptor(const std::shared_ptr<struct alloc_device_t>& alloc_device)
    :
    alloc_dev(alloc_device)
{
}

std::shared_ptr<mga::AndroidBufferHandle> mga::AndroidAllocAdaptor::alloc_buffer(
    geometry::Size size, geometry::PixelFormat pf, BufferUsage usage)
{
    buffer_handle_t buf_handle = NULL;

    int ret, stride_as_int = 0;
    int format = convert_to_android_pixel_code(pf);
    int usage_flag = convert_to_android_usage(usage);
    ret = alloc_dev->alloc(alloc_dev.get(), (int) size.width.as_uint32_t(), (int) size.height.as_uint32_t(),
                           format, usage_flag, &buf_handle, &stride_as_int);

    //todo: kdub we should not be passing an empty ptr around like this
    AndroidBufferHandleEmptyDeleter empty_del;
    AndroidBufferHandle *null_handle = NULL;
    if (( ret ) || (buf_handle == NULL) || (stride_as_int == 0))
        return std::shared_ptr<mga::AndroidBufferHandle>(null_handle, empty_del);

    /* pack ANativeWindow buffer for the handle */
    ANativeWindowBuffer buffer;
    buffer.width = (int) size.width.as_uint32_t();
    buffer.height = (int) size.height.as_uint32_t();
    buffer.stride = stride_as_int;
    buffer.handle = buf_handle;
    buffer.format = format;
    buffer.usage = usage_flag;

    /* we don't use these for refcounting buffers. however, drivers still expect to be
       able to call them */
    buffer.common.incRef = &incRef;
    buffer.common.decRef = &incRef;
    buffer.common.magic = ANDROID_NATIVE_BUFFER_MAGIC;
    buffer.common.version = sizeof(ANativeWindowBuffer);

    AndroidBufferHandleDefaultDeleter del(alloc_dev);
    auto handle = std::shared_ptr<mga::AndroidBufferHandle>(
                      new mga::AndroidBufferHandleDefault(buffer, pf, usage), del);

    return handle;
}


int mga::AndroidAllocAdaptor::convert_to_android_usage(BufferUsage usage)
{
    switch (usage)
    {
    case mga::BufferUsage::use_hardware:
        return (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER);
    case mga::BufferUsage::use_framebuffer_gles:
        return (GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_FB);
    case mga::BufferUsage::use_software:
    default:
        return -1;
    }
}

int mga::AndroidAllocAdaptor::convert_to_android_pixel_code(geom::PixelFormat mir_pixel_format)
{
    switch(mir_pixel_format)
    {
        case geom::PixelFormat::abgr_8888:
            return HAL_PIXEL_FORMAT_RGBA_8888;
        case geom::PixelFormat::xbgr_8888:
            return HAL_PIXEL_FORMAT_RGBX_8888;
        case geom::PixelFormat::bgr_888:
            return HAL_PIXEL_FORMAT_RGB_888;
        default:
            return 0;
    }
}

