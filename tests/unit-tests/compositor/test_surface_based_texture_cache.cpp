/*
 * Copyright © 2012 Canonical Ltd.
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

#include <functional>
#include <string>
#include <cstring>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/geometry/rectangle.h>
#include "mir/compositor/renderer.h"
#include "src/server/compositor/gl_renderer_factory.h"
#include "src/server/graphics/program_factory.h"
#include <mir_test/fake_shared.h>
#include <mir_test_doubles/mock_buffer.h>
#include <mir_test_doubles/mock_renderable.h>
#include <mir_test_doubles/mock_buffer_stream.h>
#include <mir/compositor/buffer_stream.h>
#include <mir_test_doubles/mock_gl.h>

using testing::SetArgPointee;
using testing::InSequence;
using testing::Return;
using testing::ReturnRef;
using testing::Pointee;
using testing::AnyNumber;
using testing::AtLeast;
using testing::_;

namespace mt=mir::test;
namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace mg=mir::graphics;

namespace
{

const GLint stub_v_shader = 1;
const GLint stub_f_shader = 2;
const GLint stub_program = 1;
const GLuint stub_texture = 1;
const GLint transform_uniform_location = 1;
const GLint alpha_uniform_location = 2;
const GLint position_attr_location = 3;
const GLint texcoord_attr_location = 4;
const GLint screen_to_gl_coords_uniform_location = 5;
const GLint tex_uniform_location = 6;
const GLint display_transform_uniform_location = 7;
const GLint centre_uniform_location = 8;
const std::string stub_info_log = "something failed!";
const size_t stub_info_log_length = stub_info_log.size();


void SetUpMockProgramData(mtd::MockGL &mock_gl)
{
    /* Uniforms and Attributes */
    EXPECT_CALL(mock_gl, glUseProgram(stub_program));

    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(tex_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(display_transform_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(transform_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(alpha_uniform_location));
    EXPECT_CALL(mock_gl, glGetAttribLocation(stub_program, _))
        .WillOnce(Return(position_attr_location));
    EXPECT_CALL(mock_gl, glGetAttribLocation(stub_program, _))
        .WillOnce(Return(texcoord_attr_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(centre_uniform_location));
}

class GLRenderer :
    public testing::Test
{
public:
    GLRenderer()
    {
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
};

}


TEST_F(GLRenderer, caches_and_uploads_texture_only_on_buffer_changes)
{
    InSequence seq;
    std::shared_ptr<mtd::MockBuffer> mock_buffer;
    testing::NiceMock<mtd::MockRenderable> renderable;
    ON_CALL(mock_renderable, buffer())
        .WillByDefault(Return(mock_buffer));

    // First render() - texture generated and uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                         _));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                         _));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(_, _))
        .Times(0);

    // Second render() - texture found in cache and not re-uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(123)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(_, _))
        .Times(0);

    // Third render() - texture found in cache but refreshed with new buffer
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)))
        .Times(0);

    // Forth render() - stale texture reuploaded following bypass
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)))
        .Times(0);

    // Fifth render() - texture found in cache and not re-uploaded
    EXPECT_CALL(*mock_buffer, id())
        .WillOnce(Return(mg::BufferID(456)));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glDeleteTextures(_, _))
        .Times(0);

    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(stub_texture)));

    cache->load_and_bind_texture(renderable);
    cache->load_and_bind_texture(renderable);
    cache->load_and_bind_texture(renderable);
    cache->invalidate();
    cache->load_and_bind_texture(renderable);
    cache->load_and_bind_texture(renderable);
}
