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
#include "mir_test_doubles/mock_buffer_bundle.h"
#include <gtest/gtest.h>
#include <stdexcept>

namespace mtd=mir::test::doubles;
namespace mg = mir::graphics;
namespace mc=mir::compositor;
namespace geom=mir::geometry;

namespace
{
class TemporaryTestBuffer : public mc::TemporaryBuffer
{
public:
    TemporaryTestBuffer(const std::shared_ptr<mg::Buffer>& buf)
        : TemporaryBuffer(buf)
    {
    }
};

class TemporaryBuffersTest : public ::testing::Test
{
public:
    TemporaryBuffersTest()
        : buffer_size{1024, 768},
          buffer_stride{1024},
          buffer_pixel_format{mir_pixel_format_abgr_8888},
          mock_buffer{std::make_shared<testing::NiceMock<mtd::MockBuffer>>(
                          buffer_size, buffer_stride, buffer_pixel_format)},
          mock_bundle{std::make_shared<testing::NiceMock<mtd::MockBufferBundle>>()}
    {
        using namespace testing;

        ON_CALL(*mock_bundle, client_acquire(_))
            .WillByDefault(InvokeArgument<0>(mock_buffer.get()));
        ON_CALL(*mock_bundle, compositor_acquire(_))
            .WillByDefault(Return(mock_buffer));
    }

    geom::Size const buffer_size;
    geom::Stride const buffer_stride;
    MirPixelFormat const buffer_pixel_format;
    std::shared_ptr<mtd::MockBuffer> const mock_buffer;
    std::shared_ptr<mtd::MockBufferBundle> mock_bundle;
};
}

TEST_F(TemporaryBuffersTest, compositor_buffer_acquires_and_releases)
{
    using namespace testing;
    EXPECT_CALL(*mock_bundle, compositor_acquire(_))
        .WillOnce(Return(mock_buffer));
    EXPECT_CALL(*mock_bundle, compositor_release(_))
        .Times(1);

    mc::TemporaryCompositorBuffer proxy_buffer(mock_bundle, 0);
}

TEST_F(TemporaryBuffersTest, snapshot_buffer_acquires_and_releases)
{
    using namespace testing;
    EXPECT_CALL(*mock_bundle, snapshot_acquire())
        .WillOnce(Return(mock_buffer));
    EXPECT_CALL(*mock_bundle, snapshot_release(_))
        .Times(1);

    mc::TemporarySnapshotBuffer proxy_buffer(mock_bundle);
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

    MirPixelFormat pixel_format;
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

TEST_F(TemporaryBuffersTest, base_test_id)
{
    TemporaryTestBuffer proxy_buffer(mock_buffer);
    EXPECT_CALL(*mock_buffer, id())
        .Times(1);

    proxy_buffer.id();
}

TEST_F(TemporaryBuffersTest, base_test_native_buffer_handle)
{
    TemporaryTestBuffer proxy_buffer(mock_buffer);
    EXPECT_CALL(*mock_buffer, native_buffer_handle())
        .Times(1);

    proxy_buffer.native_buffer_handle();
}
