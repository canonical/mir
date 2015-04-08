/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#ifndef MIR_GRAPHICS_X_BUFFER_ALLOCATOR_H_
#define MIR_GRAPHICS_X_BUFFER_ALLOCATOR_H_

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_id.h"

namespace mir
{
namespace graphics
{
namespace X
{

class BufferAllocator: public graphics::GraphicBufferAllocator
{
public:
    BufferAllocator() = default;
    virtual ~BufferAllocator() = default;

    virtual std::shared_ptr<Buffer> alloc_buffer(
        graphics::BufferProperties const& buffer_properties) override;

    std::vector<MirPixelFormat> supported_pixel_formats() override;

private:
    bool is_pixel_format_supported(MirPixelFormat format);
    std::shared_ptr<Buffer> alloc_software_buffer(
        graphics::BufferProperties const& buffer_properties);
    std::shared_ptr<Buffer> alloc_hardware_buffer(
        graphics::BufferProperties const& buffer_properties);
};

}
}
}

#endif // MIR_GRAPHICS_X_BUFFER_ALLOCATOR_H_
