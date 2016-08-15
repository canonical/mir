/*
 * Copyright © 2016 Canonical Ltd.
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

#include "src/server/graphics/nested/buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/test/doubles/stub_host_connection.h"
#include "mir/test/fake_shared.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir_toolkit/client_types_nbs.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

using namespace testing;
namespace
{
struct MockHostConnection : mtd::StubHostConnection
{
    MOCK_METHOD1(create_buffer, std::shared_ptr<MirBuffer>(mg::BufferProperties const&));
    MOCK_METHOD1(get_native_handle, MirNativeBuffer*(MirBuffer*));
    MOCK_METHOD1(get_graphics_region, MirGraphicsRegion(MirBuffer*));
};

struct NestedBuffer : Test
{
    NestedBuffer()
    {
        ON_CALL(mock_connection, create_buffer(_))
            .WillByDefault(Return(buffer));
    }
    NiceMock<MockHostConnection> mock_connection;
    mg::BufferProperties properties{{1, 1}, mir_pixel_format_abgr_8888, mg::BufferUsage::software};
    std::shared_ptr<MirBuffer> buffer;
};
}

TEST_F(NestedBuffer, creates_buffer_when_constructed)
{
    EXPECT_CALL(mock_connection, create_buffer(properties))
        .WillOnce(Return(buffer));
    mgn::Buffer buffer(mt::fake_shared(mock_connection), properties);
}

TEST_F(NestedBuffer, generates_valid_id)
{
    mgn::Buffer buffer(mt::fake_shared(mock_connection), properties);
    EXPECT_THAT(buffer.id().as_value(), Gt(0));
}

TEST_F(NestedBuffer, has_correct_properties)
{
    auto expected_stride = mir::geometry::Stride{properties.size.width.as_int() * MIR_BYTES_PER_PIXEL(properties.format)}; 
    mgn::Buffer buffer(mt::fake_shared(mock_connection), properties);
    EXPECT_THAT(buffer.size(), Eq(properties.size));
    EXPECT_THAT(buffer.pixel_format(), Eq(properties.format));
    EXPECT_THAT(buffer.stride(), Eq(expected_stride));
}

TEST_F(NestedBuffer, no_gl_support_for_now)
{
    mgn::Buffer buffer(mt::fake_shared(mock_connection), properties);
    auto native_base = buffer.native_buffer_base();
    EXPECT_THAT(native_base, Eq(nullptr));
    EXPECT_THAT(dynamic_cast<mir::renderer::gl::TextureSource*>(native_base), Eq(nullptr));
}

TEST_F(NestedBuffer, writes_to_region)
{
    unsigned int data = 0x11223344;
    MirGraphicsRegion region {
        properties.size.width.as_int(), properties.size.height.as_int(),
        properties.size.width.as_int() * MIR_BYTES_PER_PIXEL(properties.format),
        properties.format, reinterpret_cast<char*>(&data)
    };

    EXPECT_CALL(mock_connection, get_graphics_region(_))
        .WillOnce(Return(region));

    unsigned int new_data = 0x11111111;
    mgn::Buffer buffer(mt::fake_shared(mock_connection), properties);
    buffer.write(reinterpret_cast<unsigned char*>(&new_data), sizeof(new_data));
    EXPECT_THAT(data, Eq(new_data));
}

//mg::Buffer::write could be improved so that the user doesn't have to generate potentially large buffers.
TEST_F(NestedBuffer, throws_if_incorrect_sizing)
{
    EXPECT_CALL(mock_connection, get_graphics_region(_))
        .Times(0);

    unsigned int new_data = 0x11111111;
    auto too_large_size = 4 * sizeof(new_data);
    mgn::Buffer buffer(mt::fake_shared(mock_connection), properties);
    EXPECT_THROW({
        buffer.write(reinterpret_cast<unsigned char*>(&new_data), too_large_size);
    }, std::logic_error);
}

TEST_F(NestedBuffer, reads_from_region)
{
    unsigned int data = 0x11223344;
    MirGraphicsRegion region {
        properties.size.width.as_int(), properties.size.height.as_int(),
        properties.size.width.as_int() * MIR_BYTES_PER_PIXEL(properties.format),
        properties.format, reinterpret_cast<char*>(&data)
    };

    EXPECT_CALL(mock_connection, get_graphics_region(_))
        .WillOnce(Return(region));

    unsigned int read_data = 0x11111111;
    mgn::Buffer buffer(mt::fake_shared(mock_connection), properties);
    buffer.read([&] (auto pix) {
        read_data = *reinterpret_cast<decltype(data) const*>(pix);
    } );
    EXPECT_THAT(read_data, Eq(data));
}
