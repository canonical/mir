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

#ifndef MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_
#define MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_

#include <hardware/hardware.h>

#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "graphic_buffer_allocator.h"

namespace mir
{
namespace graphics
{

class BufferInitializer;

namespace android
{

class GraphicAllocAdaptor;

class AndroidGraphicBufferAllocator: public GraphicBufferAllocator, public compositor::GraphicBufferAllocator
{
public:
    AndroidGraphicBufferAllocator(
        std::shared_ptr<BufferInitializer> const& buffer_initializer);

    std::shared_ptr<compositor::Buffer> alloc_buffer(
        compositor::BufferProperties const& buffer_properties);

    std::shared_ptr<Buffer> alloc_buffer_platform(
        geometry::Size sz, geometry::PixelFormat pf, BufferUsage use);

    std::vector<geometry::PixelFormat> supported_pixel_formats();

    static BufferUsage convert_from_compositor_usage(compositor::BufferUsage usage);

private:
    const hw_module_t    *hw_module;
    std::shared_ptr<GraphicAllocAdaptor> alloc_device;
    std::shared_ptr<BufferInitializer> const buffer_initializer;
};

}
}
}
#endif /* MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_ */
