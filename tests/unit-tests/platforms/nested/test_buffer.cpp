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
#include "src/server/graphics/nested/egl_image_factory.h"
#include "src/client/buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/test/doubles/stub_host_connection.h"
#include "mir/test/doubles/mock_client_buffer.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/mock_egl.h"
#include "mir/test/fake_shared.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir_toolkit/client_types_nbs.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mir/test/gmock_fixes.h"

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

using namespace testing;
namespace
{
struct MockEglImageFactory : mgn::EglImageFactory
{
    MOCK_CONST_METHOD3(create_egl_image_from,
        std::unique_ptr<EGLImageKHR>(MirBuffer*, EGLDisplay, EGLint const*));
};

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
            .WillByDefault(Return(mirbuffer));
        ON_CALL(*client_buffer, stride())
            .WillByDefault(Return(geom::Stride{stride_with_padding}));
        ON_CALL(*client_buffer, size())
            .WillByDefault(Return(properties.size));
        ON_CALL(*client_buffer, pixel_format())
            .WillByDefault(Return(properties.format));
    }
    NiceMock<MockHostConnection> mock_connection;
    mg::BufferProperties properties{{1, 1}, mir_pixel_format_abgr_8888, mg::BufferUsage::software};

    std::shared_ptr<mtd::MockClientBuffer> client_buffer = std::make_shared<NiceMock<mtd::MockClientBuffer>>();
    std::shared_ptr<mir::client::MirBuffer> buffer = std::make_shared<mir::client::Buffer>(
        nullptr, nullptr, 0, client_buffer, nullptr, mir_buffer_usage_software);
    std::shared_ptr<MirBuffer> mirbuffer { reinterpret_cast<MirBuffer*>(buffer.get()), [](auto){}};

    unsigned int data = 0x11111111;
    int stride_with_padding = properties.size.width.as_int() * MIR_BYTES_PER_PIXEL(properties.format) + 4;
    MirGraphicsRegion region {
        properties.size.width.as_int(), properties.size.height.as_int(),
        stride_with_padding, properties.format, reinterpret_cast<char*>(&data)
    };

    MockEglImageFactory mock_image_factory;

    NiceMock<mtd::MockGL> mock_gl;
    NiceMock<mtd::MockEGL> mock_egl;
};
}

TEST_F(NestedBuffer, creates_buffer_when_constructed)
{
    EXPECT_CALL(mock_connection, create_buffer(properties))
        .WillOnce(Return(mirbuffer));
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), properties);
}

TEST_F(NestedBuffer, generates_valid_id)
{
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), properties);
    EXPECT_THAT(buffer.id().as_value(), Gt(0));
}

TEST_F(NestedBuffer, has_correct_properties)
{
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), properties);
    EXPECT_THAT(buffer.size(), Eq(properties.size));
    EXPECT_THAT(buffer.pixel_format(), Eq(properties.format));
    EXPECT_THAT(buffer.stride().as_int(), Eq(stride_with_padding));
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
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), properties);
    buffer.write(reinterpret_cast<unsigned char*>(&new_data), sizeof(new_data));
    EXPECT_THAT(data, Eq(new_data));
}

//mg::Buffer::write could be improved so that the user doesn't have to generate potentially large buffers.
TEST_F(NestedBuffer, throws_if_incorrect_sizing)
{
    auto too_large_size = 4 * sizeof(data);
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), properties);
    EXPECT_THROW({
        buffer.write(reinterpret_cast<unsigned char*>(&data), too_large_size);
    }, std::logic_error);
}

TEST_F(NestedBuffer, reads_from_region)
{
    EXPECT_CALL(mock_connection, get_graphics_region(_))
        .Times(1)
        .WillOnce(Return(region));

    unsigned int read_data = 0x11111111;
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), properties);
    buffer.read([&] (auto pix) {
        read_data = *reinterpret_cast<decltype(data) const*>(pix);
    } );
    EXPECT_THAT(read_data, Eq(data));
}

TEST_F(NestedBuffer, binds_to_texture)
{
    EGLint const expected_image_attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    EXPECT_CALL(mock_image_factory, create_egl_image_from(mirbuffer.get(), mock_egl.fake_egl_display, mtd::AttrMatches(expected_image_attrs)))
        .WillRepeatedly(InvokeWithoutArgs([] { return std::make_unique<EGLImageKHR>(); }));

    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), properties);
    auto native_base = buffer.native_buffer_base();
    ASSERT_THAT(native_base, Ne(nullptr));
    auto texture_source = dynamic_cast<mir::renderer::gl::TextureSource*>(native_base);
    ASSERT_THAT(texture_source, Ne(nullptr));

    EXPECT_CALL(mock_egl, eglGetCurrentDisplay());
    EXPECT_CALL(mock_egl, eglGetCurrentContext());
    EXPECT_CALL(*client_buffer, wait_fence(mir_read, _));
    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, _));

    texture_source->gl_bind_to_texture();
}

TEST_F(NestedBuffer, just_makes_one_bind_per_display_context_pair)
{
    int fake_context1 = 113;
    int fake_context2 = 114;

    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), properties);
    auto texture_source = dynamic_cast<mir::renderer::gl::TextureSource*>(buffer.native_buffer_base());
    ASSERT_THAT(texture_source, Ne(nullptr));

    EXPECT_CALL(mock_egl, eglGetCurrentDisplay())
        .Times(3);
    EXPECT_CALL(mock_egl, eglGetCurrentContext())
        .Times(3)
        .WillOnce(Return(&fake_context1))
        .WillOnce(Return(&fake_context1))
        .WillOnce(Return(&fake_context2));
    EXPECT_CALL(mock_image_factory, create_egl_image_from(_, _, _))
        .Times(2)
        .WillRepeatedly(InvokeWithoutArgs([] { return std::make_unique<EGLImageKHR>(); }));
    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(_, _))
        .Times(3);

    texture_source->gl_bind_to_texture();
    texture_source->gl_bind_to_texture();
    texture_source->gl_bind_to_texture();
}
