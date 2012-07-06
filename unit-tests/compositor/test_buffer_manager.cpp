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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mock_buffer.h"
#include "mock_graphic_buffer_allocator.h"

#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/graphic_buffer_allocator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{

struct EmptyDeleter
{
    template<typename T>
    void operator()(T* )
    {
    }    
};

struct MockBufferAllocationStrategy : public mc::BufferAllocationStrategy
{
    MockBufferAllocationStrategy(std::shared_ptr<mc::GraphicBufferAllocator> allocator)
            : mc::BufferAllocationStrategy(allocator)
    {
    }

    MOCK_METHOD4(
        allocate_buffers_for_bundle,
        void(geom::Width, geom::Height, mc::PixelFormat, mc::BufferBundle* bundle));
};

const geom::Width width{1024};
const geom::Height height{768};
const geom::Stride stride{geom::dim_cast<geom::Stride>(width)};
const mc::PixelFormat pixel_format{mc::PixelFormat::rgba_8888};

}

TEST(buffer_manager, create_buffer)
{
    using namespace testing;
    
    mc::MockBuffer mock_buffer{width, height, stride, pixel_format};
    std::shared_ptr<mc::MockBuffer> default_buffer(
        &mock_buffer,
        EmptyDeleter()); 
    mc::MockGraphicBufferAllocator graphic_allocator;
    std::shared_ptr<mc::GraphicBufferAllocator> allocator(&graphic_allocator, EmptyDeleter());
    MockBufferAllocationStrategy allocation_strategy(allocator);

    mc::BufferBundleManager buffer_bundle_manager(
        &allocation_strategy);

    /* note: this is somewhat of a weak test, some create_clients will create a varied amount
             of buffers */
    EXPECT_CALL(
        graphic_allocator,
        alloc_buffer(Eq(width), Eq(height), Eq(pixel_format)))
            .Times(0);

    EXPECT_CALL(allocation_strategy, allocate_buffers_for_bundle(Eq(width), Eq(height), Eq(pixel_format), _)).Times(AtLeast(1));
    
    std::shared_ptr<mc::BufferBundle> bundle{
        buffer_bundle_manager.create_buffer_bundle(
            width,
            height,
            pixel_format)};
    
    EXPECT_TRUE(bundle != nullptr);
    
}
