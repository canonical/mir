/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/graphics/android/android_alloc_adaptor.h"
#include <stdexcept>

namespace mg=mir::graphics;
namespace mga=mir::graphics::android;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

mga::AndroidAllocAdaptor::AndroidAllocAdaptor(const std::shared_ptr<struct alloc_device_t>& alloc_device)
    :
    alloc_dev(alloc_device)
{
}

#include <iostream>
struct AndroidBufferHandleDeleter
{
    AndroidBufferHandleDeleter(std::shared_ptr<alloc_device_t> alloc_dev)
        : alloc_device(alloc_dev)
    {}

    void operator()(mga::AndroidBufferHandle* t)
    {
        alloc_device->free(alloc_device.get(), t->anw_buffer.handle);
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

std::shared_ptr<mga::BufferHandle> mga::AndroidAllocAdaptor::alloc_buffer(geometry::Width width, geometry::Height height,
                              compositor::PixelFormat pf, BufferUsage usage)
{
    buffer_handle_t buf_handle = NULL;

    int ret, stride_as_int = 0;
    int format = convert_to_android_format(pf);
    int usage_flag = convert_to_android_usage(usage);
    ret = alloc_dev->alloc(alloc_dev.get(), (int) width.as_uint32_t(), (int) height.as_uint32_t(),
                            format, usage_flag, &buf_handle, &stride_as_int);

    AndroidBufferHandleEmptyDeleter empty_del;
    AndroidBufferHandle *empt = NULL;
    if (( ret ) || (buf_handle == NULL) || (stride_as_int == 0))
        return std::shared_ptr<mga::BufferHandle>(empt, empty_del);

    AndroidBufferHandleDeleter del(alloc_dev);

    ANativeWindowBuffer buffer;
    buffer.width = (int) width.as_uint32_t();
    buffer.height = (int) height.as_uint32_t();
    buffer.stride = stride_as_int;
    buffer.handle = buf_handle;

    auto handle = std::shared_ptr<mga::BufferHandle>(
            new mga::AndroidBufferHandle(buffer, pf, usage), del);

    return handle;
}


int mga::AndroidAllocAdaptor::convert_to_android_usage(BufferUsage usage)
{
    switch (usage)
    {
    case mga::BufferUsage::use_hardware:
        return (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER);
    case mga::BufferUsage::use_software:
    default:
        return -1;
    }
}

int mga::AndroidAllocAdaptor::convert_to_android_format(mc::PixelFormat pf)
{
    switch (pf)
    {
    case mc::PixelFormat::rgba_8888:
        return HAL_PIXEL_FORMAT_RGBA_8888;
    default:
        return -1;
    }
}

