/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/platforms/common/server/shm_buffer.h"
#include "mir/shm_file.h"

#include "mir/test/doubles/mock_gl.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <GLES2/gl2ext.h>
#include <endian.h>

namespace mg = mir::graphics;
namespace mgc = mir::graphics::common;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
using namespace testing;

namespace
{

struct PlatformlessShmBuffer : mgc::MemoryBackedShmBuffer
{
    PlatformlessShmBuffer(
        geom::Size const& size,
        MirPixelFormat const& pixel_format)
            : MemoryBackedShmBuffer(
                size,
                pixel_format)
    {
    }

    auto pixel_buffer() -> unsigned char const*
    {
        unsigned char const* buffer{nullptr};
        read(
            [&buffer](unsigned char const* pixels)
            {
                buffer = pixels;
            });
        return buffer;
    }

    std::shared_ptr<mg::NativeBuffer> native_buffer_handle() const override
    {
        return nullptr;
    }
};

struct ShmBufferTest : public testing::Test
{
    ShmBufferTest()
        : size{150,340},
          pixel_format{mir_pixel_format_bgr_888},
          shm_buffer{size, pixel_format}
    {
    }

    geom::Size const size;
    MirPixelFormat const pixel_format;
    PlatformlessShmBuffer shm_buffer;
    testing::NiceMock<mtd::MockGL> mock_gl;
};

}

TEST_F(ShmBufferTest, has_correct_properties)
{
    size_t const bytes_per_pixel = MIR_BYTES_PER_PIXEL(pixel_format);
    size_t const expected_stride{bytes_per_pixel * size.width.as_uint32_t()};

    EXPECT_EQ(size, shm_buffer.size());
    EXPECT_EQ(geom::Stride{expected_stride}, shm_buffer.stride());
    EXPECT_EQ(pixel_format, shm_buffer.pixel_format());
}

TEST_F(ShmBufferTest, cant_upload_bgr_888)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_bgr_888);
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, _,
                                      size.width.as_int(), size.height.as_int(),
                                      0, _, _,
                                      buf.pixel_buffer()))
                .Times(0);

    buf.bind();
}

TEST_F(ShmBufferTest, uploads_rgb_888_correctly)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_rgb_888);

    EXPECT_CALL(mock_gl, glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                                      size.width.as_int(), size.height.as_int(),
                                      0, GL_RGB, GL_UNSIGNED_BYTE,
                                      buf.pixel_buffer()));

    buf.bind();
}

TEST_F(ShmBufferTest, uploads_rgb_565_correctly)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_rgb_565);
    EXPECT_CALL(mock_gl, glPixelStorei(GL_UNPACK_ALIGNMENT,
                                       AnyOf(Eq(1),Eq(2))));
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                                      size.width.as_int(), size.height.as_int(),
                                      0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                                      buf.pixel_buffer()));

    buf.bind();
}

TEST_F(ShmBufferTest, uploads_rgba_5551_correctly)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_rgba_5551);
    EXPECT_CALL(mock_gl, glPixelStorei(GL_UNPACK_ALIGNMENT,
                                       AnyOf(Eq(1),Eq(2))));
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                      size.width.as_int(), size.height.as_int(),
                                      0, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1,
                                      buf.pixel_buffer()));

    buf.bind();
}

TEST_F(ShmBufferTest, uploads_rgba_4444_correctly)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_rgba_4444);
    EXPECT_CALL(mock_gl, glPixelStorei(GL_UNPACK_ALIGNMENT,
                                       AnyOf(Eq(1),Eq(2))));
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                      size.width.as_int(), size.height.as_int(),
                                      0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4,
                                      buf.pixel_buffer()));

    buf.bind();
}

TEST_F(ShmBufferTest, uploads_xrgb_8888_correctly)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_xrgb_8888);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
                                      size.width.as_int(), size.height.as_int(),
                                      0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                                      buf.pixel_buffer()));
#endif
    buf.bind();
}

TEST_F(ShmBufferTest, uploads_argb_8888_correctly)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_argb_8888);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
                                      size.width.as_int(), size.height.as_int(),
                                      0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                                      buf.pixel_buffer()));
#endif
    buf.bind();
}

TEST_F(ShmBufferTest, uploads_xbgr_8888_correctly)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_xbgr_8888);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                      size.width.as_int(), size.height.as_int(),
                                      0, GL_RGBA, GL_UNSIGNED_BYTE,
                                      buf.pixel_buffer()));
#endif
    buf.bind();
}

TEST_F(ShmBufferTest, uploads_abgr_8888_correctly)
{
    PlatformlessShmBuffer buf(size, mir_pixel_format_abgr_8888);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                                      size.width.as_int(), size.height.as_int(),
                                      0, GL_RGBA, GL_UNSIGNED_BYTE,
                                      buf.pixel_buffer()));
#endif
    buf.bind();
}
