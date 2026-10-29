/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gl_handles.h"
#include <mir/test/doubles/mock_gl.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;

using namespace testing;

// static_assert requirements:
static_assert(!std::is_default_constructible_v<mg::GLHandle<&glDeleteProgram>>);
static_assert(std::is_default_constructible_v<mg::ProgramHandle>);
static_assert(!std::is_copy_constructible_v<mg::GLHandle<&glDeleteProgram>>);
static_assert(!std::is_copy_constructible_v<mg::ProgramHandle>);
static_assert(!std::is_copy_assignable_v<mg::GLHandle<&glDeleteProgram>>);
static_assert(!std::is_copy_assignable_v<mg::ProgramHandle>);
static_assert(std::is_move_constructible_v<mg::GLHandle<&glDeleteProgram>>);
static_assert(std::is_move_constructible_v<mg::ProgramHandle>);
static_assert(std::is_move_assignable_v<mg::GLHandle<&glDeleteProgram>>);
static_assert(std::is_move_assignable_v<mg::ProgramHandle>);

namespace
{
struct GLHandleTest : public Test
{
    mtd::MockGL mock_gl;
};

TEST_F(GLHandleTest, adopt_existing_name_and_deletes_on_destruction)
{
    GLuint const id = 42;
    EXPECT_CALL(mock_gl, glDeleteProgram(id)).Times(1);

    {
        mg::GLHandle<&glDeleteProgram> handle{id};
        EXPECT_EQ(static_cast<GLuint>(handle), id);
    }
}

TEST_F(GLHandleTest, program_handle_allocates_on_construction_and_deletes_on_destruction)
{
    GLuint const program_id = 101;
    InSequence const s;
    EXPECT_CALL(mock_gl, glCreateProgram()).WillOnce(Return(program_id));
    EXPECT_CALL(mock_gl, glDeleteProgram(program_id)).Times(1);

    {
        mg::ProgramHandle program;
        EXPECT_EQ(static_cast<GLuint>(program), program_id);
    }
}

TEST_F(GLHandleTest, shader_handle_allocates_with_enum_and_deletes_on_destruction)
{
    GLuint const shader_id = 202;
    GLenum const shader_type = GL_VERTEX_SHADER;

    InSequence const s;
    EXPECT_CALL(mock_gl, glCreateShader(shader_type)).WillOnce(Return(shader_id));
    EXPECT_CALL(mock_gl, glDeleteShader(shader_id)).Times(1);

    {
        mg::ShaderHandle shader{shader_type};
        EXPECT_EQ(static_cast<GLuint>(shader), shader_id);
    }
}

TEST_F(GLHandleTest, bulk_allocator_and_bulk_deleter)
{
    GLuint const texture_id = 303;

    InSequence const s;
    EXPECT_CALL(mock_gl, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(texture_id));

    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(texture_id))).Times(1);

    {
        mg::TextureHandle texture;
        EXPECT_EQ(static_cast<GLuint>(texture), texture_id);
    }
}

TEST_F(GLHandleTest, moved_from_handle_deletes_exactly_once)
{
    GLuint const program_id = 404;
    InSequence const s;
    EXPECT_CALL(mock_gl, glCreateProgram()).WillOnce(Return(program_id));
    EXPECT_CALL(mock_gl, glDeleteProgram(program_id)).Times(1);

    {
        mg::ProgramHandle original;
        EXPECT_EQ(static_cast<GLuint>(original), program_id);

        mg::ProgramHandle destination{std::move(original)};
        EXPECT_EQ(static_cast<GLuint>(original), 0u);
        EXPECT_EQ(static_cast<GLuint>(destination), program_id);
    }
}

TEST_F(GLHandleTest, move_assignment_deletes_overwritten_handle)
{
    GLuint const id1 = 501;
    GLuint const id2 = 502;

    EXPECT_CALL(mock_gl, glCreateProgram()).WillOnce(Return(id1)).WillOnce(Return(id2));

    InSequence const s;
    EXPECT_CALL(mock_gl, glDeleteProgram(id1)).Times(1);
    EXPECT_CALL(mock_gl, glDeleteProgram(id2)).Times(1);

    {
        mg::ProgramHandle handle1;
        {
            mg::ProgramHandle handle2;
            handle2 = std::move(handle1);
            EXPECT_EQ(static_cast<GLuint>(handle1), id2);
            EXPECT_EQ(static_cast<GLuint>(handle2), id1);
        }
    }
}

TEST_F(GLHandleTest, reset_deletes_early_and_destructor_does_not_double_delete)
{
    GLuint const id = 601;
    InSequence const s;
    EXPECT_CALL(mock_gl, glCreateProgram()).WillOnce(Return(id));
    EXPECT_CALL(mock_gl, glDeleteProgram(id)).Times(1);

    {
        mg::ProgramHandle program;
        EXPECT_EQ(static_cast<GLuint>(program), id);
        program.reset();
        EXPECT_EQ(static_cast<GLuint>(program), 0u);
    }
}

TEST_F(GLHandleTest, buffer_handle_allocates_and_deletes)
{
    GLuint const buf_id = 701;

    InSequence const s;
    EXPECT_CALL(mock_gl, glGenBuffers(1, _)).WillOnce(SetArgPointee<1>(buf_id));
    EXPECT_CALL(mock_gl, glDeleteBuffers(1, Pointee(buf_id))).Times(1);

    {
        mg::BufferHandle buffer;
        EXPECT_EQ(static_cast<GLuint>(buffer), buf_id);
    }
}

TEST_F(GLHandleTest, renderbuffer_handle_allocates_and_deletes)
{
    GLuint const rb_id = 702;

    InSequence const s;
    EXPECT_CALL(mock_gl, glGenRenderbuffers(1, _)).WillOnce(SetArgPointee<1>(rb_id));
    EXPECT_CALL(mock_gl, glDeleteRenderbuffers(1, Pointee(rb_id))).Times(1);

    {
        mg::RenderbufferHandle renderbuffer;
        EXPECT_EQ(static_cast<GLuint>(renderbuffer), rb_id);
    }
}

TEST_F(GLHandleTest, framebuffer_handle_allocates_and_deletes)
{
    GLuint const fb_id = 703;

    InSequence const s;
    EXPECT_CALL(mock_gl, glGenFramebuffers(1, _)).WillOnce(SetArgPointee<1>(fb_id));
    EXPECT_CALL(mock_gl, glDeleteFramebuffers(1, Pointee(fb_id))).Times(1);

    {
        mg::FramebufferHandle framebuffer;
        EXPECT_EQ(static_cast<GLuint>(framebuffer), fb_id);
    }
}
}
