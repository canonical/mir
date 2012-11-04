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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/geometry/size.h>
#include <mir/graphics/gl_renderer.h>
#include <mir/graphics/renderable.h>
#include <mir/compositor/graphic_region.h>
#include <mir_test/gl_mock.h>

using testing::SetArgPointee;
using testing::InSequence;
using testing::Return;
using testing::_;

namespace mc=mir::compositor;
namespace mg=mir::graphics;

namespace
{

const GLint stub_bad_shader = 0;
const GLint stub_bad_program = 0;
const GLint stub_v_shader = 1;
const GLint stub_f_shader = 2;
const GLint stub_program = 1;
const GLint stub_vbo = 1;
const GLint stub_texture = 1;
const GLint transform_uniform_location = 1;
const GLint alpha_uniform_location = 2;
const GLint position_attr_location = 3;
const GLint texcoord_attr_location = 4;
const GLint screen_to_gl_coords_uniform_location = 5;
const GLint tex_uniform_location = 6;

class TestSetupGLRenderer :
    public testing::Test
{
public:

    TestSetupGLRenderer()
    {
        /* Vertex Shader */
        EXPECT_CALL (gl_mock, glCreateShader(GL_VERTEX_SHADER))
                .WillOnce(Return(stub_v_shader));
        EXPECT_CALL (gl_mock, glShaderSource(stub_v_shader, 1, _, 0));
        EXPECT_CALL (gl_mock, glCompileShader (stub_v_shader));
        EXPECT_CALL (gl_mock, glGetShaderiv(stub_v_shader, GL_COMPILE_STATUS, _))
                .WillOnce(SetArgPointee<2>(GL_TRUE));

        /* Fragment Shader */
        EXPECT_CALL (gl_mock, glCreateShader(GL_FRAGMENT_SHADER))
                .WillOnce(Return(stub_f_shader));
        EXPECT_CALL (gl_mock, glShaderSource(stub_f_shader, 1, _, 0));
        EXPECT_CALL (gl_mock, glCompileShader (stub_f_shader));
        EXPECT_CALL (gl_mock, glGetShaderiv(stub_f_shader, GL_COMPILE_STATUS, _))
                .WillOnce(SetArgPointee<2>(GL_TRUE));

        /* Graphics Program */
        EXPECT_CALL (gl_mock, glCreateProgram())
                .WillOnce (Return (stub_program));
        EXPECT_CALL (gl_mock, glAttachShader (stub_program, stub_v_shader));
        EXPECT_CALL (gl_mock, glAttachShader (stub_program, stub_f_shader));
        EXPECT_CALL (gl_mock, glLinkProgram (stub_program));
        EXPECT_CALL (gl_mock, glGetProgramiv (stub_program, GL_LINK_STATUS, _))
                .WillOnce (SetArgPointee<2>(GL_TRUE));

        /* Uniforms and Attributes */
        EXPECT_CALL (gl_mock, glUseProgram(stub_program));

        InSequence s;

        EXPECT_CALL(gl_mock, glGetUniformLocation(stub_program, _))
                .WillOnce(Return(screen_to_gl_coords_uniform_location));
        EXPECT_CALL(gl_mock, glGetUniformLocation(stub_program, _))
                .WillOnce(Return(tex_uniform_location));
        EXPECT_CALL(gl_mock, glGetUniformLocation(stub_program, _))
                .WillOnce(Return(transform_uniform_location));
        EXPECT_CALL(gl_mock, glGetUniformLocation(stub_program, _))
                .WillOnce(Return(alpha_uniform_location));
        EXPECT_CALL(gl_mock, glGetAttribLocation(stub_program, _))
                .WillOnce(Return(position_attr_location));
        EXPECT_CALL(gl_mock, glGetAttribLocation(stub_program, _))
                .WillOnce(Return(texcoord_attr_location));

        EXPECT_CALL(gl_mock, glUniformMatrix4fv(screen_to_gl_coords_uniform_location, 1, GL_FALSE, _));

        /* Set up the render texture */
        EXPECT_CALL(gl_mock, glGenTextures(1, _))
                .WillOnce(SetArgPointee<1>(stub_texture));
        EXPECT_CALL(gl_mock, glBindTexture(GL_TEXTURE_2D, stub_texture));
        EXPECT_CALL(gl_mock, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        EXPECT_CALL(gl_mock, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        EXPECT_CALL(gl_mock, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        EXPECT_CALL(gl_mock, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

        EXPECT_CALL(gl_mock, glUniform1i (tex_uniform_location, 0));

        /* Set up VBO */
        EXPECT_CALL(gl_mock, glGenBuffers(1, _))
                .WillOnce(SetArgPointee<1>(stub_vbo));
        EXPECT_CALL(gl_mock, glBindBuffer(GL_ARRAY_BUFFER, stub_vbo));
        EXPECT_CALL(gl_mock, glBufferData(GL_ARRAY_BUFFER, _, _, GL_STATIC_DRAW));

        /* These should go away */
        EXPECT_CALL(gl_mock, glBindBuffer(GL_ARRAY_BUFFER, 0));
        EXPECT_CALL(gl_mock, glUseProgram(0));

        renderer.reset (new mg::GLRenderer (display_size));
    }

    mir::GLMock         gl_mock;
    mir::geometry::Size display_size;
    std::unique_ptr <mg::GLRenderer> renderer;
};

class MockRenderable :
    public mg::Renderable
{
public:

    MOCK_CONST_METHOD0(top_left, mir::geometry::Point ());
    MOCK_CONST_METHOD0(size, mir::geometry::Size ());
    MOCK_CONST_METHOD0(texture, std::shared_ptr<mc::GraphicRegion> ());
    MOCK_CONST_METHOD0(transformation, glm::mat4 ());
    MOCK_CONST_METHOD0(alpha, float ());
};

class MockGraphicRegion :
    public mc::GraphicRegion
{
public:

    MOCK_CONST_METHOD0(size, mir::geometry::Size ());
    MOCK_CONST_METHOD0(stride, mir::geometry::Stride ());
    MOCK_CONST_METHOD0(pixel_format, mir::geometry::PixelFormat ());
    MOCK_METHOD0(bind_to_texture, void ());
};

}

TEST_F(TestSetupGLRenderer, TestSetUpRenderContextBeforeRenderingRenderable)
{
    MockRenderable rd;
    std::shared_ptr<MockGraphicRegion> gr (new MockGraphicRegion ());
    mir::geometry::Point tl;
    mir::geometry::Size  s;
    glm::mat4            transformation;

    tl.x = mir::geometry::X (1);
    tl.y = mir::geometry::Y (2);

    s.width = mir::geometry::Width (10);
    s.height = mir::geometry::Height (20);

    InSequence seq;

    EXPECT_CALL(rd, top_left()).WillOnce(Return(tl));
    EXPECT_CALL(rd, size()).WillOnce(Return(s));

    EXPECT_CALL(gl_mock, glUseProgram(stub_program));
    EXPECT_CALL(gl_mock, glEnable(GL_BLEND));
    EXPECT_CALL(gl_mock, glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    EXPECT_CALL(gl_mock, glActiveTexture(GL_TEXTURE0));

    EXPECT_CALL(rd, transformation())
            .WillOnce(Return(transformation));
    EXPECT_CALL(gl_mock, glUniformMatrix4fv(transform_uniform_location, 1, GL_FALSE, _));
    EXPECT_CALL(rd, alpha())
            .WillOnce(Return(0));
    EXPECT_CALL(gl_mock, glUniform1f(alpha_uniform_location, _));
    EXPECT_CALL(gl_mock, glBindBuffer(GL_ARRAY_BUFFER, stub_vbo));
    EXPECT_CALL(gl_mock, glVertexAttribPointer(position_attr_location, 3, GL_FLOAT, GL_FALSE, _, _));
    EXPECT_CALL(gl_mock, glVertexAttribPointer(texcoord_attr_location, 2, GL_FLOAT, GL_FALSE, _, _));

    EXPECT_CALL(gl_mock, glBindTexture(GL_TEXTURE_2D, stub_texture));

    EXPECT_CALL(rd, texture())
            .WillOnce(Return(gr));
    EXPECT_CALL(*gr, bind_to_texture());

    EXPECT_CALL(gl_mock, glEnableVertexAttribArray(position_attr_location));
    EXPECT_CALL(gl_mock, glEnableVertexAttribArray(texcoord_attr_location));

    EXPECT_CALL(gl_mock, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    EXPECT_CALL(gl_mock, glDisableVertexAttribArray(texcoord_attr_location));
    EXPECT_CALL(gl_mock, glDisableVertexAttribArray(position_attr_location));

    renderer->render (rd);
}
