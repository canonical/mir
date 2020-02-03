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
#include "src/platforms/common/server/egl_context_executor.h"
#include "mir/renderer/gl/context.h"

#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/mock_egl.h"

#include "check_gtest_version.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <endian.h>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgc = mir::graphics::common;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
using namespace testing;

namespace
{

class DumbGLContext : public mir::renderer::gl::Context
{
public:
    DumbGLContext(EGLContext ctx)
        : ctx{ctx}
    {
    }

    void make_current() const override
    {
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx);
    }

    void release_current() const override
    {
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

private:
    EGLDisplay const dpy{reinterpret_cast<void*>(0xdeebbeed)};
    EGLContext const ctx;
};

struct PlatformlessShmBuffer : mgc::MemoryBackedShmBuffer
{
    PlatformlessShmBuffer(
        geom::Size const& size,
        MirPixelFormat const& pixel_format,
        std::shared_ptr<mgc::EGLContextExecutor> egl_delegate)
        : MemoryBackedShmBuffer(
            size,
            pixel_format,
            std::move(egl_delegate))
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
        : size{150, 340},
          pixel_format{mir_pixel_format_bgr_888},
          egl_delegate{
            std::make_shared<mgc::EGLContextExecutor>(
                std::make_unique<DumbGLContext>(dummy))},
          shm_buffer{
            size,
            pixel_format,
            std::make_shared<mgc::EGLContextExecutor>(
                std::make_unique<DumbGLContext>(dummy))}
    {
    }

    testing::NiceMock<mtd::MockEGL> mock_egl;
    testing::NiceMock<mtd::MockGL> mock_gl;

    geom::Size const size;
    MirPixelFormat const pixel_format;
    EGLContext const dummy{reinterpret_cast<void*>(0x0011223344)};
    std::shared_ptr<mgc::EGLContextExecutor> const egl_delegate;

    PlatformlessShmBuffer shm_buffer;
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
    PlatformlessShmBuffer buf(size, mir_pixel_format_bgr_888, egl_delegate);
    EXPECT_CALL(mock_gl, glTexImage2D(GL_TEXTURE_2D, 0, _,
                                      size.width.as_int(), size.height.as_int(),
                                      0, _, _,
                                      buf.pixel_buffer()))
                .Times(0);

    buf.bind();
}

struct BufferUploadDesc
{
    geom::Size size;
    // TODO: Test uploads with stride_in_px != size.width
    int stride_in_px;
    MirPixelFormat format;
    GLenum gl_format;
    GLenum gl_type;
};

class UploadTest :
    public ShmBufferTest,
    public WithParamInterface<BufferUploadDesc>
{
};

TEST_P(UploadTest, uploads_correctly)
{
    auto const desc = GetParam();

    PlatformlessShmBuffer buf(
        desc.size, desc.format, egl_delegate);

    ExpectationSet gl_setup;
    gl_setup +=
        EXPECT_CALL(mock_gl, glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    gl_setup +=
        EXPECT_CALL(mock_gl, glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, desc.stride_in_px));

    Expectation gl_use = EXPECT_CALL(
        mock_gl,
        glTexImage2D(
            GL_TEXTURE_2D, 0,
            desc.gl_format,
            desc.size.width.as_int(), desc.size.height.as_int(),
            0,
            desc.gl_format, desc.gl_type,
            buf.pixel_buffer()));

    // Ensure we reset GL state to be polite to other users.
    EXPECT_CALL(mock_gl, glPixelStorei(GL_UNPACK_ALIGNMENT, 4))
        .After(gl_use);
    EXPECT_CALL(mock_gl, glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, 0))
        .After(gl_use);

    buf.bind();

}

namespace
{
geom::Size const default_size{245, 553};

BufferUploadDesc const test_cases[] = {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    BufferUploadDesc{
        default_size,
        default_size.width.as_int(),
        mir_pixel_format_xrgb_8888,
        GL_BGRA_EXT,
        GL_UNSIGNED_BYTE
    },
    BufferUploadDesc{
        default_size,
        default_size.width.as_int(),
        mir_pixel_format_argb_8888,
        GL_BGRA_EXT,
        GL_UNSIGNED_BYTE
    },
    BufferUploadDesc{
        default_size,
        default_size.width.as_int(),
        mir_pixel_format_xbgr_8888,
        GL_RGBA,
        GL_UNSIGNED_BYTE
    },
    BufferUploadDesc{
        default_size,
        default_size.width.as_int(),
        mir_pixel_format_abgr_8888,
        GL_RGBA,
        GL_UNSIGNED_BYTE
    },
#endif
    BufferUploadDesc {
        default_size,
        default_size.width.as_int(),
        mir_pixel_format_rgb_888,
        GL_RGB,
        GL_UNSIGNED_BYTE
    },
    BufferUploadDesc{
        default_size,
        default_size.width.as_int(),
        mir_pixel_format_rgb_565,
        GL_RGB,
        GL_UNSIGNED_SHORT_5_6_5
    },
    BufferUploadDesc{
        default_size,
        default_size.width.as_int(),
        mir_pixel_format_rgba_5551,
        GL_RGBA,
        GL_UNSIGNED_SHORT_5_5_5_1
    },
    BufferUploadDesc{
        default_size,
        default_size.width.as_int(),
        mir_pixel_format_rgba_4444,
        GL_RGBA,
        GL_UNSIGNED_SHORT_4_4_4_4
    },
};
}

namespace std
{
std::string to_string(MirPixelFormat format)
{
    // TODO: Wouldn't it be nice if these were much more usable?
    switch(format)
    {
#define ENUM_TO_STR(value) \
    case value: return #value

        ENUM_TO_STR(mir_pixel_format_invalid);
        ENUM_TO_STR(mir_pixel_format_abgr_8888);
        ENUM_TO_STR(mir_pixel_format_xbgr_8888);
        ENUM_TO_STR(mir_pixel_format_argb_8888);
        ENUM_TO_STR(mir_pixel_format_xrgb_8888);
        ENUM_TO_STR(mir_pixel_format_bgr_888);
        ENUM_TO_STR(mir_pixel_format_rgb_888);
        ENUM_TO_STR(mir_pixel_format_rgb_565);
        ENUM_TO_STR(mir_pixel_format_rgba_5551);
        ENUM_TO_STR(mir_pixel_format_rgba_4444);
#undef ENUM_TO_STR
    default:
        return "UNKNOWN MirPixelFormat";
    }
}
}

#if GTEST_AT_LEAST(1, 8, 0)
INSTANTIATE_TEST_SUITE_P(
    ShmBuffer,
    UploadTest,
    ValuesIn(test_cases),
    [](const testing::TestParamInfo<UploadTest::ParamType>& info)
    {
        return std::to_string(info.param.format);
    });
#else
// TODO: The version of gtest in 16.04 doesn't have the “print the test nicely” option.
INSTANTIATE_TEST_SUITE_P(
    ShmBuffer,
    UploadTest,
    ValuesIn(test_cases));
#endif

namespace
{
void wait_for_egl_thread(mgc::EGLContextExecutor& egl_delegate)
{
    auto egl_thread_started_promise = std::make_shared<std::promise<void>>();
    auto const egl_thread_started = egl_thread_started_promise->get_future();

    egl_delegate.spawn(
        [promise = std::move(egl_thread_started_promise)]()
        {
            promise->set_value();
        });

    if (egl_thread_started.wait_for(std::chrono::seconds{10}) != std::future_status::ready)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Timeout waiting for EGLContextDelegate"}));
    }
}
}

TEST_F(ShmBufferTest, texture_is_destroyed_on_thread_with_current_context)
{
    GLint const tex_id{0x8086};
    EXPECT_CALL(mock_gl, glGenTextures(1,_))
        .WillOnce(SetArgPointee<1>(tex_id));
    EXPECT_CALL(mock_gl, glDeleteTextures(1,Pointee(Eq(tex_id))))
        .WillOnce(InvokeWithoutArgs(
            [this]()
            {
                EXPECT_THAT(
                    mock_egl.current_contexts[std::this_thread::get_id()],
                    Ne(EGL_NO_CONTEXT));
            }));

    EGLContext const dummy_ctx{reinterpret_cast<EGLContext>(0x66221144)};
    EGLDisplay const dummy_dpy{reinterpret_cast<EGLDisplay>(0xaabbccdd)};

    {
        /* Use a locally-scoped EGLContextDelegate to make use of
         * the fact that the destructor ensures the work-queue is drained.
         */
        auto egl_delegate = std::make_shared<mgc::EGLContextExecutor>(
            std::make_unique<DumbGLContext>(reinterpret_cast<EGLContext>(42)));

        // Ensure that the EGL thread has actually spun up…
        wait_for_egl_thread(*egl_delegate);

        // Ensure we have a “context” current for creation and bind
        eglMakeCurrent(dummy_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, dummy_ctx);

        PlatformlessShmBuffer buffer{size, pixel_format, egl_delegate};

        buffer.bind();

        // Ensure this thread does *not* have a “context” current for destruction
        eglMakeCurrent(dummy_dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
}
