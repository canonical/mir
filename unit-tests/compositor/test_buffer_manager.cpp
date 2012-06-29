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
#include "mir/compositor/buffer_manager.h"
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
};

struct MockBufferManager : public mc::BufferManager
{
 public:
    explicit MockBufferManager(mc::GraphicBufferAllocator* gr_allocator) : mc::BufferManager(gr_allocator) {}
    
    MOCK_METHOD3(create_buffer, std::shared_ptr<mc::Buffer>(geom::Width, geom::Height, mc::PixelFormat));
    MOCK_METHOD1(register_buffer, bool(std::shared_ptr<mc::Buffer>));
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
    mc::BufferManager buffer_manager(&graphic_allocator);

    EXPECT_CALL(graphic_allocator, alloc_buffer(Eq(width), Eq(height), Eq(pixel_format))).
    		Times(1).WillRepeatedly(Return(default_buffer));

    std::shared_ptr<mc::Buffer> buffer = buffer_manager.create_buffer(
        width,
        height,
        pixel_format);

    EXPECT_TRUE(buffer.get() != nullptr);

}
