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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_GBM_BUFFER_ALLOCATOR_H_
#define MIR_PLATFORM_GBM_BUFFER_ALLOCATOR_H_

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_id.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wall"
#include <gbm.h>
#pragma GCC diagnostic pop

#include <memory>

namespace mir
{
namespace graphics
{
class BufferInitializer;
struct EGLExtensions;

namespace gbm
{
class BufferAllocator: public graphics::GraphicBufferAllocator
{
public:
    BufferAllocator(gbm_device* device,
                    const std::shared_ptr<BufferInitializer>& buffer_initializer);

    virtual std::shared_ptr<Buffer> alloc_buffer(
        graphics::BufferProperties const& buffer_properties);

    std::vector<geometry::PixelFormat> supported_pixel_formats();

private:
    bool is_pixel_format_supported(geometry::PixelFormat format);

    gbm_device* const device;
    std::shared_ptr<graphics::BufferInitializer> buffer_initializer;
    std::shared_ptr<EGLExtensions> const egl_extensions;

    bool bypass_env;
};

}
}
}

#endif // MIR_PLATFORM_GBM_BUFFER_ALLOCATOR_H_
