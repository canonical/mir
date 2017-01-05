/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_
#define MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/test/doubles/stub_buffer.h"

#include <vector>
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubBufferAllocator : public graphics::GraphicBufferAllocator
{
    std::shared_ptr<graphics::Buffer> alloc_buffer(
        graphics::BufferProperties const& properties)
    {
        return std::make_shared<StubBuffer>(properties);
    }

    std::shared_ptr<graphics::Buffer> alloc_buffer(geometry::Size sz, MirPixelFormat pf)
    {
        return std::make_shared<StubBuffer>(
            graphics::BufferProperties{sz, pf, graphics::BufferUsage::software});
    }

    std::shared_ptr<graphics::Buffer> alloc_buffer(geometry::Size sz, uint32_t, uint32_t)
    {
        return std::make_shared<StubBuffer>(
            graphics::BufferProperties{sz, mir_pixel_format_abgr_8888, graphics::BufferUsage::hardware});
    }

    std::vector<MirPixelFormat> supported_pixel_formats()
    {
        return {};
    }
};

}
}
}

#endif // MIR_TEST_DOUBLES_STUB_BUFFER_ALLOCATOR_H_
