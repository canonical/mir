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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/proxy_buffer.h"
#include "mir_test_doubles/mock_buffer.h"
#include <gtest/gtest.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

class TemporaryBufferTest : public ::testing::Test
{
public:
    void SetUp()
    {
        using testing::NiceMock;
        buffer_size = geom::Size{geom::Width{1024}, geom::Height{768}};
        buffer_stride = geom::Stride{1024};
        buffer_pixel_format = geom::PixelFormat{geom::PixelFormat::abgr_8888};
        buffer = std::make_shared<NiceMock<mtd::MockBuffer>>(buffer_size, buffer_stride, buffer_pixel_format);
    }
    std::shared_ptr<mtd::MockBuffer> buffer;
    geom::Size buffer_size;
    geom::Stride buffer_stride;
    geom::PixelFormat buffer_pixel_format;
};

TEST_F(TemporaryBufferTest, buffer_has_ownership)
{
    {
        mc::TemporaryBuffer proxy_buffer(buffer);
        EXPECT_EQ(buffer.use_count(), 2);
    }
    EXPECT_EQ(buffer.use_count(), 1);
} 

TEST_F(TemporaryBufferTest, test_size)
{
    mc::TemporaryBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, size())
        .Times(1);

    geom::Size size;
    size = proxy_buffer.size();
    EXPECT_EQ(buffer_size, size);
} 

TEST_F(TemporaryBufferTest, test_stride)
{
    mc::TemporaryBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, stride())
        .Times(1);

    geom::Stride stride;
    stride = proxy_buffer.stride();
    EXPECT_EQ(buffer_stride, stride);
} 

TEST_F(TemporaryBufferTest, test_pixel_format)
{
    mc::TemporaryBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, pixel_format())
        .Times(1);

    geom::PixelFormat pixel_format;
    pixel_format = proxy_buffer.pixel_format();
    EXPECT_EQ(buffer_pixel_format, pixel_format);
} 

TEST_F(TemporaryBufferTest, bind_to_texture)
{
    mc::TemporaryBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, bind_to_texture())
        .Times(1);

    proxy_buffer.bind_to_texture();
} 

TEST_F(TemporaryBufferTest, get_ipc_package)
{
    mc::TemporaryBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, get_ipc_package())
        .Times(1);

    proxy_buffer.get_ipc_package();
} 

TEST_F(TemporaryBufferTest, test_id)
{
    EXPECT_CALL(*buffer, id())
        .Times(1);
    mc::TemporaryBuffer proxy_buffer(buffer);

    proxy_buffer.id();
} 
