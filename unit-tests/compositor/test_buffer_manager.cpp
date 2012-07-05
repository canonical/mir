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
    MockBufferAllocationStrategy(mc::GraphicBufferAllocator* allocator)
            : mc::BufferAllocationStrategy(allocator)
    {
    }

    MOCK_METHOD4(
        allocate_buffers_for_bundle,
        void(geom::Width, geom::Height, mc::PixelFormat, mc::BufferBundle* bundle));
};

struct MockGraphicBufferAllocator : mc::GraphicBufferAllocator
{
 public:
    MOCK_METHOD3(alloc_buffer, std::shared_ptr<mc::Buffer>(geom::Width, geom::Height, mc::PixelFormat));
    MOCK_METHOD1(free_buffer, void(std::shared_ptr<mc::Buffer>));
};

const geom::Width width{1024};
const geom::Height height{768};
const geom::Stride stride{geom::dim_cast<geom::Stride>(width)};
const mc::PixelFormat pixel_format{mc::PixelFormat::rgba_8888};

struct MockBuffer : public mc::Buffer
{
 public:
	MockBuffer()
	{
	    using namespace testing;
		ON_CALL(*this, width()).       WillByDefault(Return(::width));
		ON_CALL(*this, height()).      WillByDefault(Return(::height));
		ON_CALL(*this, stride()).      WillByDefault(Return(::stride));
		ON_CALL(*this, pixel_format()).WillByDefault(Return(::pixel_format));
	}

    MOCK_CONST_METHOD0(width, geom::Width());
    MOCK_CONST_METHOD0(height, geom::Height());
    MOCK_CONST_METHOD0(stride, geom::Stride());
    MOCK_CONST_METHOD0(pixel_format, mc::PixelFormat());

    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(bind_to_texture, void());
};
}

TEST(buffer_manager, create_buffer)
{
    using namespace testing;
    
    MockBuffer mock_buffer;
    std::shared_ptr<MockBuffer> default_buffer(
        &mock_buffer,
        EmptyDeleter()); 
    MockGraphicBufferAllocator graphic_allocator;
    MockBufferAllocationStrategy allocation_strategy(&graphic_allocator);
    mc::BufferBundleManager buffer_bundle_manager(
        &allocation_strategy,
        &graphic_allocator);

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
    EXPECT_FALSE(buffer_bundle_manager.is_empty()); 

    /* this will destroy the buffers that are of shared_ptr type, but this is resource allocation is tested in the bufferManagerClient tests */
    buffer_bundle_manager.destroy_buffer_bundle(bundle);
    EXPECT_TRUE(buffer_bundle_manager.is_empty()); 
}
