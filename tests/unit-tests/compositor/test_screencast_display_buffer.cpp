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

#include "src/server/compositor/screencast_display_buffer.h"
#include "src/server/compositor/queueing_schedule.h"
#include "mir/renderer/gl/texture_target.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_gl_buffer.h"
#include "mir/test/doubles/stub_gl_buffer.h"
#include "mir/test/doubles/mock_gl.h"
#include "mir/test/doubles/stub_renderable.h"
#include "mir/test/doubles/stub_display.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mc = mir::compositor;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
namespace mg = mir::graphics;

using namespace testing;

namespace
{

struct ScreencastDisplayBufferTest : testing::Test
{
    void SetUp() override
    {
        free_queue.schedule(mt::fake_shared(stub_buffer));
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    mc::QueueingSchedule free_queue;
    mc::QueueingSchedule ready_queue;
    mtd::StubGLBuffer stub_buffer;
    mtd::StubDisplay stub_display{1};
    geom::Size const default_size{300,400};
    geom::Rectangle const default_rect{{100,100}, {800,600}};
    MirMirrorMode const default_mirror_mode{mir_mirror_mode_vertical};
};

}

TEST_F(ScreencastDisplayBufferTest, cleans_up_gl_resources)
{
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

    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   default_mirror_mode, free_queue,
                                   ready_queue, stub_display};
}

TEST_F(ScreencastDisplayBufferTest, cleans_up_gl_resources_on_construction_failure)
{
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

    EXPECT_THROW({
        mc::ScreencastDisplayBuffer db(default_rect, default_size,
                                       default_mirror_mode, free_queue,
                                       ready_queue, stub_display);
    }, std::runtime_error);
}

TEST_F(ScreencastDisplayBufferTest, sets_render_buffer_size_to_supplied_size)
{
    /* Set the buffer as rendering target */
    EXPECT_CALL(mock_gl,
                    glRenderbufferStorage(_, _,
                        default_size.width.as_int(),
                        default_size.height.as_int()));

    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   default_mirror_mode, free_queue,
                                   ready_queue, stub_display};
}

TEST_F(ScreencastDisplayBufferTest, renders_to_supplied_buffer)
{
    struct MockTargetBuffer : mtd::MockBuffer,
                              mir::renderer::gl::TextureTarget
    {
        using MockBuffer::MockBuffer;
        MOCK_METHOD0(bind_for_write, void());
        MOCK_METHOD0(commit, void());
    };
 
    testing::NiceMock<MockTargetBuffer> mock_buffer{
        default_rect.size, geom::Stride{100}, mir_pixel_format_xbgr_8888};

    mc::QueueingSchedule free_queue;
    free_queue.schedule(mt::fake_shared(mock_buffer));

    InSequence s;
    /* Set the buffer as rendering target */
    EXPECT_CALL(mock_buffer, bind_for_write());
    EXPECT_CALL(mock_gl,
                glViewport(0, 0,
                           mock_buffer.size().width.as_int(),
                           mock_buffer.size().height.as_int()));
    /* Restore previous viewport on exit */
    EXPECT_CALL(mock_gl, glViewport(0, 0, 0, 0));

    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   default_mirror_mode, free_queue,
                                   ready_queue, stub_display};
    db.bind();
}

TEST_F(ScreencastDisplayBufferTest, throws_if_cannot_write_to_supplied_buffer)
{
    testing::NiceMock<mtd::MockGLBuffer> mock_buffer{
        default_rect.size, geom::Stride{100}, mir_pixel_format_xbgr_8888};

    mc::QueueingSchedule free_queue;
    free_queue.schedule(mt::fake_shared(mock_buffer));

    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   default_mirror_mode, free_queue,
                                   ready_queue, stub_display};
    EXPECT_THROW({
        db.bind();
    }, std::invalid_argument);
}

TEST_F(ScreencastDisplayBufferTest, forces_rendering_to_complete_on_swap)
{
    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   default_mirror_mode, free_queue,
                                   ready_queue, stub_display};

    Mock::VerifyAndClearExpectations(&mock_gl);
    EXPECT_CALL(mock_gl, glFinish());

    db.bind();
    db.swap_buffers();
}

TEST_F(ScreencastDisplayBufferTest, rejects_attempt_to_optimize)
{
    mg::RenderableList renderables{
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>(),
        std::make_shared<mtd::StubRenderable>()};

    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   default_mirror_mode, free_queue,
                                   ready_queue, stub_display};

    EXPECT_FALSE(db.overlay(renderables));
}

TEST_F(ScreencastDisplayBufferTest, does_not_throw_on_multiple_make_current_calls)
{
    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   default_mirror_mode, free_queue,
                                   ready_queue, stub_display};

    EXPECT_NO_THROW(db.make_current());
    EXPECT_NO_THROW(db.make_current());
}

TEST_F(ScreencastDisplayBufferTest, schedules_onto_ready_queue)
{
    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   default_mirror_mode, free_queue,
                                   ready_queue, stub_display};

    db.bind();
    db.swap_buffers();

    ASSERT_THAT(ready_queue.num_scheduled(), Gt(0u));

    auto ready_buffer = ready_queue.next_buffer();
    auto const expected_buffer = mt::fake_shared(stub_buffer);

    EXPECT_THAT(ready_buffer, Eq(expected_buffer));
}

TEST_F(ScreencastDisplayBufferTest, uses_requested_mirror_mode)
{
    MirMirrorMode const expected_mirror_mode{mir_mirror_mode_horizontal};
    glm::mat2 const expected_transformation(-1, 0,
                                             0, 1);
    mc::ScreencastDisplayBuffer db{default_rect, default_size,
                                   expected_mirror_mode, free_queue,
                                   ready_queue, stub_display};

    EXPECT_THAT(db.transformation(), Eq(expected_transformation));
}

