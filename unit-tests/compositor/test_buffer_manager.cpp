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
#include "mir/graphics/framebuffer_backend.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{

struct EmptyDeleter
{
    template<typename T>
    void operator()(T* )
    {
    }    
};

struct MockFramebufferBackend : mg::framebuffer_backend
{
 public:
    MOCK_METHOD0(render, void ());
};

struct MockBuffer : public mc::Buffer
{
 public:
    MOCK_CONST_METHOD0(get_width, uint32_t());
    MOCK_CONST_METHOD0(get_height, uint32_t());
    MOCK_CONST_METHOD0(get_stride, uint32_t());
    MOCK_CONST_METHOD0(get_pixel_format, mc::PixelFormat());
};

struct MockBufferManager : public mc::BufferManager
{
 public:
    explicit MockBufferManager(mg::framebuffer_backend* framebuffer) : mc::BufferManager(framebuffer) {}
    
    MOCK_METHOD3(create_buffer, std::shared_ptr<mc::Buffer>(uint32_t, uint32_t, mc::PixelFormat));
    MOCK_METHOD1(register_buffer, bool(std::shared_ptr<mc::Buffer>));
};

}

TEST(buffer_manager, create_buffer)
{
    using namespace testing;
    using ::testing::_;
    using ::testing::Return;
    
    static const uint32_t kWidth{1024};
    static const uint32_t kHeight{768};
    static const uint32_t kStride{kWidth};
    static const mc::PixelFormat kPixelFormat{mc::PixelFormat::rgba_8888};
    MockBuffer mock_buffer;

    ON_CALL(mock_buffer, get_width()).
            WillByDefault(Return(kWidth));
    ON_CALL(mock_buffer, get_height()).
            WillByDefault(Return(kHeight));
    ON_CALL(mock_buffer, get_stride()).
            WillByDefault(Return(kStride));
    ON_CALL(mock_buffer, get_pixel_format()).
            WillByDefault(Return(kPixelFormat));
    
    std::shared_ptr<MockBuffer> default_buffer(
        &mock_buffer,
        EmptyDeleter());
    DefaultValue< std::shared_ptr<mc::Buffer> >::Set(default_buffer);
    
    MockFramebufferBackend graphics;
    MockBufferManager buffer_manager(&graphics);

    std::shared_ptr<mc::Buffer> buffer = buffer_manager.create_buffer(
        kWidth,
        kHeight,
        kPixelFormat);

    EXPECT_TRUE(buffer.get() != nullptr);
    EXPECT_EQ(kWidth, buffer->get_width());
    EXPECT_EQ(kHeight, buffer->get_height());
    EXPECT_EQ(kPixelFormat, buffer->get_pixel_format());
    EXPECT_EQ(kStride, buffer->get_stride());
    
}
