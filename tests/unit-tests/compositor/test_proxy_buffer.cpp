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

class ProxyBufferTest : public ::testing::Test
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

TEST_F(ProxyBufferTest, test_size)
{
    mc::ProxyBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, size())
        .Times(1);

    geom::Size size;
    EXPECT_NO_THROW({
        size = proxy_buffer.size();
    });
    EXPECT_EQ(buffer_size, size);

    buffer.reset();

    EXPECT_THROW({
        proxy_buffer.size();
    }, std::runtime_error);
} 

TEST_F(ProxyBufferTest, test_stride)
{
    mc::ProxyBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, stride())
        .Times(1);

    geom::Stride stride;
    EXPECT_NO_THROW({
        stride = proxy_buffer.stride();
    });
    EXPECT_EQ(buffer_stride, stride);

    buffer.reset();

    EXPECT_THROW({
        proxy_buffer.stride();
    }, std::runtime_error);
} 

TEST_F(ProxyBufferTest, test_pixel_format)
{
    mc::ProxyBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, pixel_format())
        .Times(1);

    geom::PixelFormat pixel_format;
    EXPECT_NO_THROW({
        pixel_format = proxy_buffer.pixel_format();
    });
    EXPECT_EQ(buffer_pixel_format, pixel_format);

    buffer.reset();

    EXPECT_THROW({
        proxy_buffer.pixel_format();
    }, std::runtime_error);
} 

TEST_F(ProxyBufferTest, bind_to_texture)
{
    mc::ProxyBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, bind_to_texture())
        .Times(1);

    EXPECT_NO_THROW({
        proxy_buffer.bind_to_texture();
    });

    buffer.reset();

    EXPECT_THROW({
        proxy_buffer.bind_to_texture();
    }, std::runtime_error);
} 

TEST_F(ProxyBufferTest, get_ipc_package)
{
    mc::ProxyBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, get_ipc_package())
        .Times(1);

    EXPECT_NO_THROW({
        proxy_buffer.get_ipc_package();
    });

    buffer.reset();

    EXPECT_THROW({
        proxy_buffer.get_ipc_package();
    }, std::runtime_error);
} 

TEST_F(ProxyBufferTest, test_id)
{
    mc::ProxyBuffer proxy_buffer(buffer);
    EXPECT_CALL(*buffer, id())
        .Times(1);

    mc::BufferID id;
    EXPECT_NO_THROW({
        id = proxy_buffer.id();
    });

    buffer.reset();

    EXPECT_THROW({
        proxy_buffer.id();
    }, std::runtime_error);
} 
