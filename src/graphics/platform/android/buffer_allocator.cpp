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

#include "mir_platform/android/buffer_allocator.h"
#include "mir_platform/android/android_alloc_adaptor.h"

#include <stdexcept>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

mg::AndroidBufferAllocator::AndroidBufferAllocator()
{
    int err;

    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
    if (err < 0)
    {
        throw std::runtime_error("Could not open hardware module");
    }

    alloc_device = std::shared_ptr<mg::GraphicAllocAdaptor>(new AndroidAllocAdaptor(hw_module));
}

std::unique_ptr<mc::Buffer> mg::AndroidBufferAllocator::alloc_buffer(
   geom::Width width, geom::Height height, mc::PixelFormat pf)
{
    return std::unique_ptr<mc::Buffer>(new AndroidBuffer(alloc_device, width, height, pf));
}

std::unique_ptr<mc::GraphicBufferAllocator> mg::create_buffer_allocator()
{
    return std::unique_ptr<AndroidBufferAllocator>(new AndroidBufferAllocator());
}


