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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/graphic_buffer_allocator.h"

#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{

class StubBuffer : public mc::Buffer
{
public:
    geom::Size size() const { return buf_size; }
    geom::Stride stride() const { return geom::Stride(); }
    geom::PixelFormat pixel_format() const { return buf_pixel_format; }
    std::shared_ptr<mc::BufferIPCPackage> get_ipc_package() const { return std::shared_ptr<mc::BufferIPCPackage>(); }
    void bind_to_texture() {}

    static geom::Size const buf_size;
    static geom::PixelFormat const buf_pixel_format;
};

geom::Size const StubBuffer::buf_size{geom::Width{100}, geom::Height{121}};
geom::PixelFormat const StubBuffer::buf_pixel_format{geom::PixelFormat::rgbx_8888};

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    std::shared_ptr<mc::Buffer> alloc_buffer(mc::BufferProperties const&)
    {
        return std::unique_ptr<mc::Buffer>(new StubBuffer());
    }

    std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return std::vector<geom::PixelFormat>();
    }
};

struct DoubleBufferAllocationStrategyTest : testing::Test
{
    DoubleBufferAllocationStrategyTest()
        : stub_allocator{std::make_shared<StubGraphicBufferAllocator>()}
    {
    }

    std::shared_ptr<mc::GraphicBufferAllocator> const stub_allocator;
};

}

TEST_F(DoubleBufferAllocationStrategyTest, create_swapper_returns_actual_properties_from_buffer)
{
    geom::Size const size{geom::Width{10},geom::Height{20}};
    geom::PixelFormat const pf{geom::PixelFormat::rgba_8888};
    mc::BufferUsage const usage{mc::BufferUsage::hardware};

    mc::DoubleBufferAllocationStrategy strategy{stub_allocator};
    mc::BufferProperties const properties{size, pf, usage};
    mc::BufferProperties actual_properties;

    auto swapper = strategy.create_swapper(actual_properties, properties);

    EXPECT_EQ(StubBuffer::buf_size, actual_properties.size);
    EXPECT_EQ(StubBuffer::buf_pixel_format, actual_properties.format);
    EXPECT_EQ(usage, actual_properties.usage);
}
