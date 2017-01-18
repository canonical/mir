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
#include "mir_test_framework/stub_platform_native_buffer.h"
#include "mir_toolkit/client_types.h"

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
        return std::make_shared<StubBuffer>(std::make_shared<mir_test_framework::NativeBuffer>(properties), properties.size);
    }

    std::shared_ptr<graphics::Buffer> alloc_software_buffer(geometry::Size sz, MirPixelFormat pf)
    {
        graphics::BufferProperties properties{sz, pf, graphics::BufferUsage::software};
        return std::make_shared<StubBuffer>(std::make_shared<mir_test_framework::NativeBuffer>(properties), sz);
    }

    std::shared_ptr<graphics::Buffer> alloc_buffer(geometry::Size sz, uint32_t, uint32_t flags)
    {
        graphics::BufferProperties properties{sz, mir_pixel_format_abgr_8888, graphics::BufferUsage::hardware};
        if (mir_buffer_usage_software == static_cast<MirBufferUsage>(flags))
            properties.usage = graphics::BufferUsage::software; 
        return std::make_shared<StubBuffer>(std::make_shared<mir_test_framework::NativeBuffer>(properties), sz);
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
