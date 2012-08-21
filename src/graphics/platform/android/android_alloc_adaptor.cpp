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
namespace mc=mir::compositor;
namespace geom=mir::geometry;

mg::AndroidAllocAdaptor::AndroidAllocAdaptor(std::shared_ptr<struct alloc_device_t> alloc_device)
 :
alloc_dev(alloc_device)
{ 
}

struct BufferHandleDeleter
{
    BufferHandleDeleter(std::shared_ptr<alloc_device_t> alloc_dev)
     : alloc_device(alloc_dev)
    {}

    void operator()(mg::AndroidBufferHandle* t) 
    {
        alloc_device->free(alloc_device.get(), t->handle);
        delete t; 
    }
    private:
        std::shared_ptr<alloc_device_t> alloc_device;
};

bool mg::AndroidAllocAdaptor::alloc_buffer(std::shared_ptr<BufferHandle>& handle, geom::Stride& stride, geom::Width width, geom::Height height,
                                          compositor::PixelFormat pf, BufferUsage usage)
{
    /* todo: to go away */
    buffer_handle_t buf_handle;

    int ret, stride_as_int;
    int format = convert_to_android_format(pf);
    int usage_flag = convert_to_android_usage(usage);
    ret = alloc_dev->alloc(alloc_dev.get(), (int) width.as_uint32_t(), (int) height.as_uint32_t(), format, usage_flag, &buf_handle, &stride_as_int);

    if (( ret ) || (buf_handle == NULL) || (stride_as_int == 0))
        return false;

    BufferHandleDeleter del(alloc_dev);
    handle = std::shared_ptr<mg::BufferHandle>(new mg::AndroidBufferHandle(buf_handle), del);
    stride = geom::Stride(stride_as_int);

    return true;
}

bool mg::AndroidAllocAdaptor::inspect_buffer(char *, int)
{
    return false;
}

int mg::AndroidAllocAdaptor::convert_to_android_usage(BufferUsage usage)
{
    switch (usage)
    {
        case mg::BufferUsage::use_hardware:
            return (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER);
        case mg::BufferUsage::use_software:
        default:
            return -1;
    }
}

int mg::AndroidAllocAdaptor::convert_to_android_format(mc::PixelFormat pf)
{
    switch (pf)
    {
        case mc::PixelFormat::rgba_8888:
            return HAL_PIXEL_FORMAT_RGBA_8888;
        default:
            return -1;
    }
}

