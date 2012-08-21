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

#include "mir_platform/android/android_alloc_adaptor.h"
#include <stdexcept>
namespace mg=mir::graphics;
namespace geom=mir::geometry;

mg::AndroidAllocAdaptor::AndroidAllocAdaptor(std::shared_ptr<struct alloc_device_t> alloc_device)
 :
alloc_dev(alloc_device)
{ 
}

bool mg::AndroidAllocAdaptor::alloc_buffer(BufferData&, geom::Stride&, geom::Width width, geom::Height height,
                                          compositor::PixelFormat, BufferUsage)
{
    int ret;
    int stride;
    buffer_handle_t buf_handle;
    ret = alloc_dev->alloc(alloc_dev.get(), (int) width.as_uint32_t(), (int) height.as_uint32_t(), 4, 0x300, &buf_handle, &stride);
    if ( ret )
        return false;

    return true;
}

bool mg::AndroidAllocAdaptor::free_buffer(BufferData)
{
    return false;
}

bool mg::AndroidAllocAdaptor::inspect_buffer(char *, int)
{
    return false;
}
