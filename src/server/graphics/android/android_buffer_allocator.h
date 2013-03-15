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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_
#define MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_

#include <hardware/hardware.h>

#include "mir/compositor/graphic_buffer_allocator.h"
#include "graphic_alloc_adaptor.h"

namespace mir
{
namespace compositor
{
class BufferIDUniqueGenerator;
}
namespace graphics
{
namespace android
{

class AndroidBufferAllocator: public compositor::GraphicBufferAllocator
{
public:
    AndroidBufferAllocator();

    virtual std::shared_ptr<compositor::Buffer> alloc_buffer(
        compositor::BufferProperties const& buffer_properties);

    std::vector<geometry::PixelFormat> supported_pixel_formats();

private:
    const hw_module_t    *hw_module;
    std::shared_ptr<GraphicAllocAdaptor> alloc_device;
};

}
}
}
#endif /* MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_ */
