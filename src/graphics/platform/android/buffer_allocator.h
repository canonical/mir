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

#include "mir/graphics/platform.h"
#include "mir/compositor/graphic_buffer_allocator.h"

#include "android_buffer.h"

namespace mir
{
namespace graphics
{

class AndroidBufferAllocator: public mc::GraphicBufferAllocator
{
public:
    AndroidBufferAllocator();
    ~AndroidBufferAllocator();

    virtual std::unique_ptr<compositor::Buffer> alloc_buffer(
        geometry::Width w, geometry::Height h, compositor::PixelFormat pf)
    {
        return std::unique_ptr<compositor::Buffer>( new AndroidBuffer(w, h, pf) );
    }
private:
    const hw_module_t    *hw_module;
    struct alloc_device_t *alloc_device;

};

}
}
