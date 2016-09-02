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

#include "src/server/graphics/nested/native_buffer.h"
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
#include "mir/renderer/sw/pixel_source.h"
#include "mir_toolkit/client_types_nbs.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "mir/test/gmock_fixes.h"

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;
namespace mrg = mir::renderer::gl;

using namespace testing;
namespace
{
struct MockEglImageFactory : mgn::EglImageFactory
{
    MOCK_CONST_METHOD3(create_egl_image_from, mgn::EGLImageUPtr(mgn::NativeBuffer&, EGLDisplay, EGLint const*));
};

struct MockNativeBuffer : mgn::NativeBuffer
{
    MOCK_CONST_METHOD0(client_handle, MirBuffer*());
    MOCK_METHOD0(get_native_handle, MirNativeBuffer*());
    MOCK_METHOD0(get_graphics_region, MirGraphicsRegion());
    MOCK_CONST_METHOD0(size, geom::Size());
    MOCK_CONST_METHOD0(format, MirPixelFormat());
    MOCK_METHOD2(sync, void(MirBufferAccess, std::chrono::nanoseconds));
};

struct MockHostConnection : mtd::StubHostConnection
{
    MOCK_METHOD1(create_buffer, std::shared_ptr<mgn::NativeBuffer>(mg::BufferProperties const&));
};

struct NestedBuffer : Test
{
    NestedBuffer()
    {
        ON_CALL(mock_connection, create_buffer(_))
            .WillByDefault(Return(client_buffer));
        ON_CALL(*client_buffer, size())
            .WillByDefault(Return(sw_properties.size));
        ON_CALL(*client_buffer, format())
            .WillByDefault(Return(sw_properties.format));
        ON_CALL(*client_buffer, get_graphics_region())
            .WillByDefault(Return(region));
    }
    NiceMock<MockHostConnection> mock_connection;
    mg::BufferProperties sw_properties{{1, 1}, mir_pixel_format_abgr_8888, mg::BufferUsage::software};
    mg::BufferProperties hw_properties{{1, 1}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};

    std::shared_ptr<MockNativeBuffer> client_buffer = std::make_shared<NiceMock<MockNativeBuffer>>();

    unsigned int data = 0x11111111;
    int stride_with_padding = sw_properties.size.width.as_int() * MIR_BYTES_PER_PIXEL(sw_properties.format) + 4;
    MirGraphicsRegion region {
        sw_properties.size.width.as_int(), sw_properties.size.height.as_int(),
        stride_with_padding, sw_properties.format, reinterpret_cast<char*>(&data)
    };

    MockEglImageFactory mock_image_factory;

    NiceMock<mtd::MockGL> mock_gl;
    NiceMock<mtd::MockEGL> mock_egl;
};
}

TEST_F(NestedBuffer, creates_buffer_when_constructed)
{
    EXPECT_CALL(mock_connection, create_buffer(sw_properties))
        .WillOnce(Return(client_buffer));
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);
}

TEST_F(NestedBuffer, generates_valid_id)
{
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);
    EXPECT_THAT(buffer.id().as_value(), Gt(0));
}

TEST_F(NestedBuffer, has_correct_properties)
{
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);
    EXPECT_THAT(buffer.size(), Eq(sw_properties.size));
    EXPECT_THAT(buffer.pixel_format(), Eq(sw_properties.format));
}

TEST_F(NestedBuffer, sw_support_if_requested)
{

    {
        mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);
        EXPECT_THAT(dynamic_cast<mrs::PixelSource*>(buffer.native_buffer_base()), Ne(nullptr));
        EXPECT_THAT(dynamic_cast<mrg::TextureSource*>(buffer.native_buffer_base()), Ne(nullptr));
    }

    {
        mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), hw_properties);
        EXPECT_THAT(dynamic_cast<mrs::PixelSource*>(buffer.native_buffer_base()), Eq(nullptr));
        EXPECT_THAT(dynamic_cast<mrg::TextureSource*>(buffer.native_buffer_base()), Ne(nullptr));
    }
}

TEST_F(NestedBuffer, writes_to_region)
{
    unsigned int data = 0x11223344;
    MirGraphicsRegion region {
        sw_properties.size.width.as_int(), sw_properties.size.height.as_int(),
        sw_properties.size.width.as_int() * MIR_BYTES_PER_PIXEL(sw_properties.format),
        sw_properties.format, reinterpret_cast<char*>(&data)
    };

    unsigned int new_data = 0x11111111;
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);

    EXPECT_CALL(*client_buffer, get_graphics_region())
        .WillOnce(Return(region));
    auto pixel_source = dynamic_cast<mir::renderer::software::PixelSource*>(buffer.native_buffer_base());
    ASSERT_THAT(pixel_source, Ne(nullptr));
    pixel_source->write(reinterpret_cast<unsigned char*>(&new_data), sizeof(new_data));
    EXPECT_THAT(data, Eq(new_data));
}

TEST_F(NestedBuffer, checks_for_null_vaddr)
{
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);

    MirGraphicsRegion region { 1, 1, 1, sw_properties.format, nullptr };
    EXPECT_CALL(*client_buffer, get_graphics_region())
        .WillOnce(Return(region));
    auto pixel_source = dynamic_cast<mir::renderer::software::PixelSource*>(buffer.native_buffer_base());
    ASSERT_THAT(pixel_source, Ne(nullptr));

    unsigned int new_data = 0x11111111;
    EXPECT_THROW({
        pixel_source->write(reinterpret_cast<unsigned char*>(&new_data), sizeof(new_data));
    }, std::logic_error);
}

//mg::Buffer::write could be improved so that the user doesn't have to generate potentially large buffers.
TEST_F(NestedBuffer, throws_if_incorrect_sizing)
{
    auto too_large_size = 4 * sizeof(data);
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);
    auto pixel_source = dynamic_cast<mir::renderer::software::PixelSource*>(buffer.native_buffer_base());
    ASSERT_THAT(pixel_source, Ne(nullptr));
    EXPECT_THROW({
        pixel_source->write(reinterpret_cast<unsigned char*>(&data), too_large_size);
    }, std::logic_error);
}

TEST_F(NestedBuffer, reads_from_region)
{
    unsigned int read_data = 0x11111111;
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);

    EXPECT_CALL(*client_buffer, get_graphics_region())
        .WillOnce(Return(region));
    auto pixel_source = dynamic_cast<mir::renderer::software::PixelSource*>(buffer.native_buffer_base());
    ASSERT_THAT(pixel_source, Ne(nullptr));
    pixel_source->read([&] (auto pix) {
        read_data = *reinterpret_cast<decltype(data) const*>(pix);
    } );
    EXPECT_THAT(read_data, Eq(data));
}

TEST_F(NestedBuffer, binds_to_texture)
{
    EGLint const expected_image_attrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    EXPECT_CALL(mock_image_factory, create_egl_image_from(Ref(*client_buffer), mock_egl.fake_egl_display, mtd::AttrMatches(expected_image_attrs)))
        .WillRepeatedly(InvokeWithoutArgs([] { return std::make_unique<EGLImageKHR>(); }));

    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), hw_properties);
    auto native_base = buffer.native_buffer_base();
    ASSERT_THAT(native_base, Ne(nullptr));
    auto texture_source = dynamic_cast<mir::renderer::gl::TextureSource*>(native_base);
    ASSERT_THAT(texture_source, Ne(nullptr));

    EXPECT_CALL(mock_egl, eglGetCurrentDisplay());
    EXPECT_CALL(mock_egl, eglGetCurrentContext());
    EXPECT_CALL(*client_buffer, sync(mir_read, _));
    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, _));

    texture_source->gl_bind_to_texture();
}

TEST_F(NestedBuffer, just_makes_one_bind_per_display_context_pair)
{
    int fake_context1 = 113;
    int fake_context2 = 114;

    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), hw_properties);
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

TEST_F(NestedBuffer, has_correct_stride)
{
    mgn::Buffer buffer(mt::fake_shared(mock_connection), mt::fake_shared(mock_image_factory), sw_properties);
    auto pixel_source = dynamic_cast<mir::renderer::software::PixelSource*>(buffer.native_buffer_base());
    ASSERT_THAT(pixel_source, Ne(nullptr));
    EXPECT_THAT(pixel_source->stride().as_int(), Eq(stride_with_padding));
}
