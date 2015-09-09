/*
 * Copyright © 2014 Canonical Ltd.
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

#include "src/server/compositor/screencast_display_buffer.h"

#include "mir/test/doubles/mock_gl_buffer.h"
#include "mir/test/doubles/stub_gl_buffer.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_renderable.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
namespace mg = mir::graphics;

namespace
{

struct ScreencastDisplayBufferTest : testing::Test
{
    testing::NiceMock<mtd::MockGL> mock_gl;
};

}

TEST_F(ScreencastDisplayBufferTest, cleans_up_gl_resources)
{
    using namespace testing;
    GLuint const texture{11};
    GLuint const renderbuffer{12};
    GLuint const framebuffer{13};

    EXPECT_CALL(mock_gl, glGenTextures(1,_))
        .WillOnce(SetArgPointee<1>(texture));
    EXPECT_CALL(mock_gl, glGenRenderbuffers(1,_))
        .WillOnce(SetArgPointee<1>(renderbuffer));
    EXPECT_CALL(mock_gl, glGenFramebuffers(1,_))
        .WillOnce(SetArgPointee<1>(framebuffer));

    EXPECT_CALL(mock_gl, glDeleteTextures(1,Pointee(texture)));
    EXPECT_CALL(mock_gl, glDeleteRenderbuffers(1,Pointee(renderbuffer)));
    EXPECT_CALL(mock_gl, glDeleteFramebuffers(1,Pointee(framebuffer)));

    geom::Rectangle const rect{{100,100}, {800,600}};
    mtd::StubGLBuffer stub_buffer;

    mc::ScreencastDisplayBuffer db{rect, stub_buffer};
}

TEST_F(ScreencastDisplayBufferTest, cleans_up_gl_resources_on_construction_failure)
{
    using namespace testing;
    GLuint const texture{11};
    GLuint const renderbuffer{12};
    GLuint const framebuffer{13};

    EXPECT_CALL(mock_gl, glGenTextures(1,_))
        .WillOnce(SetArgPointee<1>(texture));
    EXPECT_CALL(mock_gl, glGenRenderbuffers(1,_))
        .WillOnce(SetArgPointee<1>(renderbuffer));
    EXPECT_CALL(mock_gl, glGenFramebuffers(1,_))
        .WillOnce(SetArgPointee<1>(framebuffer));

    EXPECT_CALL(mock_gl, glCheckFramebufferStatus(_))
        .WillOnce(Return(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT));

    EXPECT_CALL(mock_gl, glDeleteTextures(1,Pointee(texture)));
    EXPECT_CALL(mock_gl, glDeleteRenderbuffers(1,Pointee(renderbuffer)));
    EXPECT_CALL(mock_gl, glDeleteFramebuffers(1,Pointee(framebuffer)));

    geom::Rectangle const rect{{100,100}, {800,600}};
    mtd::StubGLBuffer stub_buffer;

    EXPECT_THROW({
        mc::ScreencastDisplayBuffer db(rect, stub_buffer);
    }, std::runtime_error);
}

TEST_F(ScreencastDisplayBufferTest, sets_render_buffer_size_to_supplied_buffer_size)
{
    using namespace testing;

    geom::Rectangle const rect{{100,100}, {800,600}};
    testing::NiceMock<mtd::MockGLBuffer> mock_buffer{
        rect.size, geom::Stride{100}, mir_pixel_format_xbgr_8888};

    /* Set the buffer as rendering target */
    EXPECT_CALL(mock_gl,
                glRenderbufferStorage(_, _,
                           mock_buffer.size().width.as_int(),
                           mock_buffer.size().height.as_int()));

    mc::ScreencastDisplayBuffer db{rect, mock_buffer};
}

TEST_F(ScreencastDisplayBufferTest, renders_to_supplied_buffer)
{
    using namespace testing;

    geom::Rectangle const rect{{100,100}, {800,600}};
    testing::NiceMock<mtd::MockGLBuffer> mock_buffer{
        rect.size, geom::Stride{100}, mir_pixel_format_xbgr_8888};

    InSequence s;
    /* Set the buffer as rendering target */
    EXPECT_CALL(mock_buffer, gl_bind_to_texture());
    EXPECT_CALL(mock_gl,
                glViewport(0, 0,
                           mock_buffer.size().width.as_int(),
                           mock_buffer.size().height.as_int()));
    /* Restore previous viewport on exit */
    EXPECT_CALL(mock_gl, glViewport(0, 0, 0, 0));

    mc::ScreencastDisplayBuffer db{rect, mock_buffer};
    db.make_current();
}

TEST_F(ScreencastDisplayBufferTest, forces_rendering_to_complete_on_swap)
{
    using namespace testing;

    geom::Rectangle const rect{{100,100}, {800,600}};
    mtd::StubGLBuffer stub_buffer;

    mc::ScreencastDisplayBuffer db{rect, stub_buffer};

    Mock::VerifyAndClearExpectations(&mock_gl);
    EXPECT_CALL(mock_gl, glFinish());

    db.gl_swap_buffers();
}

TEST_F(ScreencastDisplayBufferTest, rejects_attempt_to_optimize)
{
    geom::Rectangle const rect{{100,100}, {800,600}};
    mtd::StubGLBuffer stub_buffer;

    mg::RenderableList renderables{
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>()};

    mc::ScreencastDisplayBuffer db{rect, stub_buffer};

    EXPECT_FALSE(db.post_renderables_if_optimizable(renderables));
}

