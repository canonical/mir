/*
 * Copyright © 2022 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "mir/renderer/gl/basic_buffer_render_target.h"

#include "mir/test/doubles/null_gl_context.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <set>

namespace mr = mir::renderer;
namespace mrg = mir::renderer::gl;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{

struct BasicBufferRenderTarget : Test
{
    NiceMock<mtd::MockGL> mock_gl;
    mtd::NullGLContext ctx;
    int const reasonable_width = 24, reasonable_height = 32;
    geom::Size reasonable_size{reasonable_width, reasonable_height};
    MirPixelFormat const reasonable_pixel_format{mir_pixel_format_argb_8888};
    mtd::StubBuffer reasonable_buffer{{
        reasonable_size,
        reasonable_pixel_format,
        mg::BufferUsage::software}};
};

}

TEST_F(BasicBufferRenderTarget, set_buffer_always_allocs_correct_storage)
{
    mrg::BasicBufferRenderTarget render_target{mt::fake_shared(ctx)};
    EXPECT_CALL(mock_gl, glRenderbufferStorage(_, _, reasonable_width, reasonable_height));
    render_target.set_buffer(mt::fake_shared(reasonable_buffer), reasonable_size);
    render_target.swap_buffers();
    Mock::VerifyAndClearExpectations(&mock_gl);
    int const other_width = 124, other_height = 88;
    mtd::StubBuffer other_buffer{{
        {other_width, other_height},
        reasonable_pixel_format,
        mg::BufferUsage::software}};
    EXPECT_CALL(mock_gl, glRenderbufferStorage(_, _, other_width, other_height));
    render_target.set_buffer(mt::fake_shared(other_buffer), {other_width, other_height});
    render_target.swap_buffers();
}

TEST_F(BasicBufferRenderTarget, sets_gl_viewport_on_buffer_size_change)
{
    mrg::BasicBufferRenderTarget render_target{mt::fake_shared(ctx)};
    EXPECT_CALL(mock_gl, glViewport(0, 0, reasonable_width, reasonable_height));
    render_target.set_buffer(mt::fake_shared(reasonable_buffer), reasonable_size);
    Mock::VerifyAndClearExpectations(&mock_gl);
    int const other_width = 124, other_height = 88;
    mtd::StubBuffer other_buffer{{
        {other_width, other_height},
        reasonable_pixel_format,
        mg::BufferUsage::software}};
    EXPECT_CALL(mock_gl, glViewport(0, 0, other_width, other_height));
    render_target.set_buffer(mt::fake_shared(other_buffer), {other_width, other_height});
}

TEST_F(BasicBufferRenderTarget, cleans_up_framebuffers_and_renderbuffers)
{
    std::set<GLuint> framebuffers;
    GLuint framebuffer_count{0};
    ON_CALL(mock_gl, glGenFramebuffers(1, _)).WillByDefault(Invoke([&](GLsizei, GLuint* result)
        {
            *result = framebuffer_count++;
            framebuffers.insert(*result);
        }));
    ON_CALL(mock_gl, glDeleteFramebuffers(1, _)).WillByDefault(Invoke([&](GLsizei, GLuint const* id)
        {
            framebuffers.erase(*id);
        }));
    std::set<GLuint> renderbuffers;
    GLuint renderbuffer_count{0};
    ON_CALL(mock_gl, glGenRenderbuffers(1, _)).WillByDefault(Invoke([&](GLsizei, GLuint* result)
        {
            *result = renderbuffer_count++;
            renderbuffers.insert(*result);
        }));
    ON_CALL(mock_gl, glDeleteRenderbuffers(1, _)).WillByDefault(Invoke([&](GLsizei, GLuint const* id)
        {
            renderbuffers.erase(*id);
        }));
    {
        mrg::BasicBufferRenderTarget render_target{mt::fake_shared(ctx)};
        render_target.make_current();
        render_target.set_buffer(mt::fake_shared(reasonable_buffer), reasonable_size);
        render_target.bind();
        render_target.swap_buffers();
        int const other_width = 124, other_height = 88;
        mtd::StubBuffer other_buffer{{
            {other_width, other_height},
            reasonable_pixel_format,
            mg::BufferUsage::software}};
        render_target.set_buffer(mt::fake_shared(other_buffer), {other_width, other_height});
        render_target.bind();
        render_target.swap_buffers();
    }
    EXPECT_THAT(framebuffer_count, Gt(0));
    EXPECT_THAT(framebuffers.size(), Eq(0));
    EXPECT_THAT(renderbuffer_count, Gt(0));
    EXPECT_THAT(renderbuffers.size(), Eq(0));
}

TEST_F(BasicBufferRenderTarget, reads_pixels)
{
    mrg::BasicBufferRenderTarget render_target{mt::fake_shared(ctx)};
    render_target.set_buffer(mt::fake_shared(reasonable_buffer), reasonable_size);
    EXPECT_CALL(mock_gl, glReadPixels(0, 0, reasonable_width, reasonable_height, _, _, _));
    render_target.swap_buffers();
}

TEST_F(BasicBufferRenderTarget, throws_on_invalid_buffer)
{
    mrg::BasicBufferRenderTarget render_target{mt::fake_shared(ctx)};
    EXPECT_THROW({
        mtd::StubBuffer buffer({
            {55, 66}, // wrong size
            reasonable_pixel_format,
            mg::BufferUsage::software});
        render_target.set_buffer(mt::fake_shared(buffer), {10, 20});
        render_target.swap_buffers();
    }, std::logic_error);
    EXPECT_THROW({
        mtd::StubBuffer buffer({
            reasonable_size,
            mir_pixel_format_abgr_8888, // wrong format
            mg::BufferUsage::software});
        render_target.set_buffer(mt::fake_shared(buffer), reasonable_size);
        render_target.swap_buffers();
    }, std::logic_error);
}
