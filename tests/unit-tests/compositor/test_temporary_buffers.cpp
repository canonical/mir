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

#include "src/server/compositor/temporary_buffers.h"
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/mock_swapper.h"
#include <gtest/gtest.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

namespace 
{
class TemporaryTestBuffer : public mc::TemporaryBuffer
{
public:
    TemporaryTestBuffer(const std::shared_ptr<mc::Buffer>& buf)
    {
        buffer = buf;
    }
};

class TemporaryBuffersTest : public ::testing::Test
{
public:
    void SetUp()
    {
        using namespace testing;

        buffer_size = geom::Size{geom::Width{1024}, geom::Height{768}};
        buffer_stride = geom::Stride{1024};
        buffer_pixel_format = geom::PixelFormat{geom::PixelFormat::abgr_8888};
        mock_buffer = std::make_shared<NiceMock<mtd::MockBuffer>>(buffer_size, buffer_stride, buffer_pixel_format);
        mock_swapper = std::make_shared<NiceMock<mtd::MockSwapper>>(mock_buffer);

        ON_CALL(*mock_swapper, client_acquire(_,_))
            .WillByDefault(SetArgReferee<0>(mock_buffer));
        ON_CALL(*mock_swapper, compositor_acquire(_,_))
            .WillByDefault(SetArgReferee<0>(mock_buffer));
    }

    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    std::shared_ptr<mtd::MockSwapper> mock_swapper;
    geom::Size buffer_size;
    geom::Stride buffer_stride;
    geom::PixelFormat buffer_pixel_format;
};
}

TEST_F(TemporaryBuffersTest, client_buffer_acquires_and_releases)
{
    using namespace testing;
    EXPECT_CALL(*mock_swapper, client_acquire(_,_))
        .Times(1);
    EXPECT_CALL(*mock_swapper, client_release(_))
        .Times(1);

    mc::TemporaryClientBuffer proxy_buffer(mock_swapper);
} 

TEST_F(TemporaryBuffersTest, client_buffer_handles_swapper_destruction)
{
    using namespace testing;
    EXPECT_CALL(*mock_swapper, client_acquire(_,_))
        .Times(1);

    mc::TemporaryClientBuffer proxy_buffer(mock_swapper);
    mock_swapper.reset();
} 

TEST_F(TemporaryBuffersTest, compositor_buffer_acquires_and_releases)
{
    using namespace testing;
    EXPECT_CALL(*mock_swapper, compositor_acquire(_,_))
        .Times(1);
    EXPECT_CALL(*mock_swapper, compositor_release(_))
        .Times(1);

    mc::TemporaryCompositorBuffer proxy_buffer(mock_swapper);
} 

TEST_F(TemporaryBuffersTest, compositor_buffer_handles_swapper_destruction)
{
    using namespace testing;
    EXPECT_CALL(*mock_swapper, compositor_acquire(_,_))
        .Times(1);

    mc::TemporaryCompositorBuffer proxy_buffer(mock_swapper);
    mock_swapper.reset();
} 

TEST_F(TemporaryBuffersTest, base_test_size)
{
    TemporaryTestBuffer proxy_buffer(mock_buffer);
    EXPECT_CALL(*mock_buffer, size())
        .Times(1);

    geom::Size size;
    size = proxy_buffer.size();
    EXPECT_EQ(buffer_size, size);
} 

TEST_F(TemporaryBuffersTest, base_test_stride)
{
    TemporaryTestBuffer proxy_buffer(mock_buffer);
    EXPECT_CALL(*mock_buffer, stride())
        .Times(1);

    geom::Stride stride;
    stride = proxy_buffer.stride();
    EXPECT_EQ(buffer_stride, stride);
} 

TEST_F(TemporaryBuffersTest, base_test_pixel_format)
{
    TemporaryTestBuffer proxy_buffer(mock_buffer);
    EXPECT_CALL(*mock_buffer, pixel_format())
        .Times(1);

    geom::PixelFormat pixel_format;
    pixel_format = proxy_buffer.pixel_format();
    EXPECT_EQ(buffer_pixel_format, pixel_format);
} 

TEST_F(TemporaryBuffersTest, base_bind_to_texture)
{
    TemporaryTestBuffer proxy_buffer(mock_buffer);
    EXPECT_CALL(*mock_buffer, bind_to_texture())
        .Times(1);

    proxy_buffer.bind_to_texture();
} 

TEST_F(TemporaryBuffersTest, base_get_ipc_package)
{
    TemporaryTestBuffer proxy_buffer(mock_buffer);
    EXPECT_CALL(*mock_buffer, get_ipc_package())
        .Times(1);

    proxy_buffer.get_ipc_package();
} 

TEST_F(TemporaryBuffersTest, base_test_id)
{
    TemporaryTestBuffer proxy_buffer(mock_buffer);
    EXPECT_CALL(*mock_buffer, id())
        .Times(1);

    proxy_buffer.id();
}
