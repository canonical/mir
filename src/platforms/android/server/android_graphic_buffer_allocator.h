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

#ifndef MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_
#define MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_

#include <hardware/hardware.h>
#include "mir_toolkit/mir_native_buffer.h" 

#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "graphic_buffer_allocator.h"

namespace mir
{
namespace graphics
{

class EGLExtensions;

namespace android
{

class GraphicAllocAdaptor;
class DeviceQuirks;

class AndroidGraphicBufferAllocator: public GraphicBufferAllocator, public graphics::GraphicBufferAllocator
{
public:
    AndroidGraphicBufferAllocator(std::shared_ptr<DeviceQuirks> const& quirks);

    std::shared_ptr<graphics::Buffer> alloc_buffer(
        graphics::BufferProperties const& buffer_properties) override;

    std::unique_ptr<graphics::Buffer> reconstruct_from(MirNativeBuffer* anwb, MirPixelFormat);

    std::shared_ptr<graphics::Buffer> alloc_buffer_platform(
        geometry::Size sz, MirPixelFormat pf, BufferUsage use) override;

    std::vector<MirPixelFormat> supported_pixel_formats() override;

    static BufferUsage convert_from_compositor_usage(graphics::BufferUsage usage);

private:
    const hw_module_t    *hw_module;
    std::shared_ptr<GraphicAllocAdaptor> alloc_device;
    std::shared_ptr<EGLExtensions> const egl_extensions;
};

}
}
}
#endif /* MIR_PLATFORM_ANDROID_ANDROID_BUFFER_ALLOCATOR_H_ */
