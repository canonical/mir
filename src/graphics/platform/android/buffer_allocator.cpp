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


/* from android */
#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <stdexcept>

#include "mir/graphics/platform.h"
#include "mir/compositor/graphic_buffer_allocator.h"

#include "android_buffer.h"

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace mir
{
namespace graphics
{

class AndroidBufferAllocator: public mc::GraphicBufferAllocator
{
public:
    AndroidBufferAllocator();

    virtual std::unique_ptr<mc::Buffer> alloc_buffer(
        geom::Width w, geom::Height h, mc::PixelFormat pf)
    {
        return std::unique_ptr<mc::Buffer>( new mg::AndroidBuffer(w, h, pf) );
    }
private:
    const hw_module_t    *hw_module;
    struct alloc_device_t *alloc_device;

};

}
}

mg::AndroidBufferAllocator::AndroidBufferAllocator()
{
    int err;

    err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
    if (err < 0)
    {
        throw std::runtime_error("Could not open hardware module");
    }

    err = hw_module->methods->open(hw_module, GRALLOC_HARDWARE_GPU0, (struct hw_device_t**) &alloc_device);
    if(err < 0)
    {
        throw std::runtime_error("Could not open hardware module");
    }
 
}

std::unique_ptr<mc::GraphicBufferAllocator> mg::create_buffer_allocator()
{
    return std::unique_ptr<AndroidBufferAllocator>(new AndroidBufferAllocator());
}
