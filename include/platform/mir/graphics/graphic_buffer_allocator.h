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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_GRAPHIC_BUFFER_ALLOCATOR_H_
#define MIR_GRAPHICS_GRAPHIC_BUFFER_ALLOCATOR_H_

#include "mir/graphics/buffer.h"

#include <vector>
#include <memory>

namespace mir
{
namespace graphics
{

struct BufferProperties;

/**
 * Interface to graphic buffer allocation.
 */
class GraphicBufferAllocator
{
public:
    virtual ~GraphicBufferAllocator() = default;

    /**
     * Allocates a buffer.
     *
     * \param [in] buffer_properties the properties the allocated buffer should have
     */
    virtual std::shared_ptr<Buffer> alloc_buffer(
        BufferProperties const& buffer_properties) = 0;

    /**
     * The supported buffer pixel formats.
     */
    virtual std::vector<geometry::PixelFormat> supported_pixel_formats() = 0;

protected:
    GraphicBufferAllocator() = default;
    GraphicBufferAllocator(const GraphicBufferAllocator&) = delete;
    GraphicBufferAllocator& operator=(const GraphicBufferAllocator&) = delete;
};

}
}
#endif // MIR_GRAPHICS_GRAPHIC_BUFFER_ALLOCATOR_H_
