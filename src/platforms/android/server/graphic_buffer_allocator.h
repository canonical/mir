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

#ifndef MIR_PLATFORM_ANDROID_GRAPHIC_BUFFER_ALLOCATOR_H_
#define MIR_PLATFORM_ANDROID_GRAPHIC_BUFFER_ALLOCATOR_H_

#include <hardware/hardware.h>
#include "mir_toolkit/mir_native_buffer.h" 

#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/graphic_buffer_allocator.h"

namespace mir
{
namespace graphics
{

class EGLExtensions;

namespace android
{

class Gralloc;
class DeviceQuirks;
class CommandStreamSyncFactory;

class GraphicBufferAllocator: public graphics::GraphicBufferAllocator
{
public:
    GraphicBufferAllocator(
        std::shared_ptr<CommandStreamSyncFactory> const& cmdstream_sync_factory,
        std::shared_ptr<DeviceQuirks> const& quirks);

    std::shared_ptr<graphics::Buffer> alloc_buffer(
        graphics::BufferProperties const& buffer_properties) override;

    std::unique_ptr<graphics::Buffer> reconstruct_from(MirNativeBuffer* anwb, MirPixelFormat);

    std::shared_ptr<graphics::Buffer> alloc_framebuffer(
        geometry::Size sz, MirPixelFormat pf);

    std::vector<MirPixelFormat> supported_pixel_formats() override;

private:
    const hw_module_t    *hw_module;
    std::shared_ptr<Gralloc> alloc_device;
    std::shared_ptr<EGLExtensions> const egl_extensions;
    std::shared_ptr<CommandStreamSyncFactory> const cmdstream_sync_factory;
};

}
}
}
#endif /* MIR_PLATFORM_ANDROID_GRAPHIC_BUFFER_ALLOCATOR_H_ */
