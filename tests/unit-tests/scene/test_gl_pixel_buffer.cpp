/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "src/server/scene/gl_pixel_buffer.h"
#include "mir/graphics/gl_context.h"

#include "mir/test/doubles/mock_buffer.h"
#include "mir/test/doubles/mock_gl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <GLES2/gl2ext.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace ms = mir::scene;
namespace mtd = mir::test::doubles;

namespace
{

struct MockGLContext : public mg::GLContext
{
    ~MockGLContext() noexcept {}
    MOCK_CONST_METHOD0(make_current, void());
    MOCK_CONST_METHOD0(release_current, void());
};

struct WrappingGLContext : public mg::GLContext
{
    WrappingGLContext(mg::GLContext& wrapped)
        : wrapped(wrapped)
    {
    }
    ~WrappingGLContext() noexcept {}
    void make_current() const { wrapped.make_current(); }
    void release_current() const { wrapped.release_current(); }

    mg::GLContext& wrapped;
};

class GLPixelBufferTest : public ::testing::Test
{
public:
    GLPixelBufferTest()
        : context{new WrappingGLContext{mock_context}}
    {
        using namespace testing;

        ON_CALL(mock_buffer, size())
            .WillByDefault(Return(geom::Size{51, 71}));
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    testing::NiceMock<mtd::MockBuffer> mock_buffer;
    MockGLContext mock_context;
    std::unique_ptr<WrappingGLContext> context;
};

ACTION(FillPixels)
{
    auto const pixels = static_cast<uint32_t*>(arg6);
    size_t const width = arg2;
    size_t const height = arg3;

    for (uint32_t i = 0; i < width * height; ++i)
    {
        pixels[i] = i;
    }
}

ACTION(FillPixelsRGBA)
{
    auto const pixels = static_cast<uint32_t*>(arg6);
    size_t const width = arg2;
    size_t const height = arg3;

    for (uint32_t i = 0; i < width * height; ++i)
    {
        pixels[i] = ((i << 16) & 0x00ff0000) | /* Move R to new position */
                    ((i) & 0x0000ff00) |       /* G remains at same position */
                    ((i >> 16) & 0x000000ff) | /* Move B to new position */
                    ((i) & 0xff000000);        /* A remains at same position */
    }
}

}

TEST_F(GLPixelBufferTest, returns_empty_if_not_initialized)
{
    ms::GLPixelBuffer pixels{std::move(context)};

    EXPECT_EQ(geom::Size(), pixels.size());
    EXPECT_EQ(geom::Stride(), pixels.stride());
}

TEST_F(GLPixelBufferTest, returns_data_from_bgra_buffer_texture)
{
    using namespace testing;
    GLuint const tex{10};
    GLuint const fbo{20};
    uint32_t const width{mock_buffer.size().width.as_uint32_t()};
    uint32_t const height{mock_buffer.size().height.as_uint32_t()};

    {
        InSequence s;

        /* The GL context is made current */
        EXPECT_CALL(mock_context, make_current());

        /* The texture and framebuffer are prepared */
        EXPECT_CALL(mock_gl, glGenTextures(_,_))
            .WillOnce(SetArgPointee<1>(tex));
        EXPECT_CALL(mock_gl, glBindTexture(_,tex));
        EXPECT_CALL(mock_gl, glGenFramebuffers(_,_))
            .WillOnce(SetArgPointee<1>(fbo));
        EXPECT_CALL(mock_gl, glBindFramebuffer(_,fbo));

        /* The buffer texture is read as BGRA */
        EXPECT_CALL(mock_buffer, gl_bind_to_texture());
        EXPECT_CALL(mock_gl, glFramebufferTexture2D(_,_,_,tex,0));
        EXPECT_CALL(mock_gl, glReadPixels(0, 0, width, height,
                                          GL_BGRA_EXT, GL_UNSIGNED_BYTE, _))
            .WillOnce(FillPixels());

        /* at destruction */
        EXPECT_CALL(mock_context, make_current());
        EXPECT_CALL(mock_gl, glDeleteTextures(_,_));
        EXPECT_CALL(mock_gl, glDeleteFramebuffers(_,_));
    }

    ms::GLPixelBuffer pixels{std::move(context)};

    pixels.fill_from(mock_buffer);
    auto data = pixels.as_argb_8888();

    EXPECT_EQ(mock_buffer.size(), pixels.size());
    EXPECT_EQ(geom::Stride{width * 4}, pixels.stride());

    /* Check that data has been properly y-flipped */
    EXPECT_EQ(1,
              static_cast<uint32_t const*>(data)[width * (height - 1) + 1]);
    EXPECT_EQ(width * (height / 2),
              static_cast<uint32_t const*>(data)[width * (height / 2)]);
    EXPECT_EQ(width * (height - 1),
              static_cast<uint32_t const*>(data)[0]);
    EXPECT_EQ(width - 1,
              static_cast<uint32_t const*>(data)[width * height - 1]);
}

TEST_F(GLPixelBufferTest, returns_data_from_rgba_buffer_texture)
{
    using namespace testing;
    GLuint const tex{10};
    GLuint const fbo{20};
    uint32_t const width{mock_buffer.size().width.as_uint32_t()};
    uint32_t const height{mock_buffer.size().height.as_uint32_t()};

    {
        InSequence s;

        /* The GL context is made current */
        EXPECT_CALL(mock_context, make_current());

        /* The texture and framebuffer are prepared */
        EXPECT_CALL(mock_gl, glGenTextures(_,_))
            .WillOnce(SetArgPointee<1>(tex));
        EXPECT_CALL(mock_gl, glBindTexture(_,tex));
        EXPECT_CALL(mock_gl, glGenFramebuffers(_,_))
            .WillOnce(SetArgPointee<1>(fbo));
        EXPECT_CALL(mock_gl, glBindFramebuffer(_,fbo));

        /* Try to read the FBO as BGRA but fail */
        EXPECT_CALL(mock_buffer, gl_bind_to_texture());
        EXPECT_CALL(mock_gl, glFramebufferTexture2D(_,_,_,tex,0));
        EXPECT_CALL(mock_gl, glGetError())
            .WillOnce(Return(GL_NO_ERROR));
        EXPECT_CALL(mock_gl, glReadPixels(0, 0, width, height,
                                          GL_BGRA_EXT, GL_UNSIGNED_BYTE, _));
        EXPECT_CALL(mock_gl, glGetError())
            .WillOnce(Return(GL_INVALID_ENUM));
        /* Read as RGBA */
        EXPECT_CALL(mock_gl, glReadPixels(0, 0, width, height,
                                          GL_RGBA, GL_UNSIGNED_BYTE, _))
            .WillOnce(FillPixelsRGBA());

        /* at destruction */
        EXPECT_CALL(mock_context, make_current());
        EXPECT_CALL(mock_gl, glDeleteTextures(_,_));
        EXPECT_CALL(mock_gl, glDeleteFramebuffers(_,_));
    }

    ms::GLPixelBuffer pixels{std::move(context)};

    pixels.fill_from(mock_buffer);
    auto data = pixels.as_argb_8888();

    EXPECT_EQ(mock_buffer.size(), pixels.size());
    EXPECT_EQ(geom::Stride{width * 4}, pixels.stride());

    /* Check that data has been properly y-flipped and converted to argb_8888 */
    EXPECT_EQ(1,
              static_cast<uint32_t const*>(data)[width * (height - 1) + 1]);
    EXPECT_EQ(width * (height / 2),
              static_cast<uint32_t const*>(data)[width * (height / 2)]);
    EXPECT_EQ(width * (height - 1),
              static_cast<uint32_t const*>(data)[0]);
    EXPECT_EQ(width - 1,
              static_cast<uint32_t const*>(data)[width * height - 1]);
}
