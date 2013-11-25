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

#include <functional>
#include <string>
#include <cstring>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/geometry/rectangle.h>
#include <src/server/compositor/renderer.h>
#include "src/server/compositor/gl_renderer_factory.h"
#include <mir_test/fake_shared.h>
#include <mir_test_doubles/mock_buffer.h>
#include <mir_test_doubles/mock_compositing_criteria.h>
#include <mir_test_doubles/mock_buffer_stream.h>
#include <mir/compositor/buffer_stream.h>
#include <mir_test_doubles/mock_gl.h>

using testing::SetArgPointee;
using testing::InSequence;
using testing::Return;
using testing::ReturnRef;
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
const GLint stub_vbo = 1;
const GLint stub_texture = 1;
const GLint transform_uniform_location = 1;
const GLint alpha_uniform_location = 2;
const GLint position_attr_location = 3;
const GLint texcoord_attr_location = 4;
const GLint screen_to_gl_coords_uniform_location = 5;
const GLint tex_uniform_location = 6;
const std::string stub_info_log = "something failed!";
const size_t stub_info_log_length = stub_info_log.size();

void ExpectShaderCompileFailure(const GLint shader, mtd::MockGL &mock_gl)
{
    EXPECT_CALL(mock_gl, glGetShaderiv(shader, GL_COMPILE_STATUS, _))
        .WillOnce(SetArgPointee<2>(GL_FALSE));
}

void ExpectShaderCompileSuccess(const GLint shader, mtd::MockGL &mock_gl)
{
    EXPECT_CALL(mock_gl, glGetShaderiv(shader, GL_COMPILE_STATUS, _))
        .WillOnce(SetArgPointee<2>(GL_TRUE));
}

void SetUpMockVertexShader(mtd::MockGL &mock_gl, const std::function<void(const GLint, mtd::MockGL &)> &shader_compile_expectation)
{
    /* Vertex Shader */
    EXPECT_CALL(mock_gl, glCreateShader(GL_VERTEX_SHADER))
        .WillOnce(Return(stub_v_shader));
    EXPECT_CALL(mock_gl, glShaderSource(stub_v_shader, 1, _, 0));
    EXPECT_CALL(mock_gl, glCompileShader(stub_v_shader));
    shader_compile_expectation(stub_v_shader, mock_gl);
}

void SetUpMockFragmentShader(mtd::MockGL &mock_gl, const std::function<void(const GLint, mtd::MockGL &)> &shader_compile_expectation)
{
    /* Fragment Shader */
    EXPECT_CALL(mock_gl, glCreateShader(GL_FRAGMENT_SHADER))
        .WillOnce(Return(stub_f_shader));
    EXPECT_CALL(mock_gl, glShaderSource(stub_f_shader, 1, _, 0));
    EXPECT_CALL(mock_gl, glCompileShader(stub_f_shader));
    shader_compile_expectation(stub_f_shader, mock_gl);
}

void ExpectProgramLinkFailure(const GLint program, mtd::MockGL &mock_gl)
{
    EXPECT_CALL(mock_gl, glGetProgramiv(program, GL_LINK_STATUS, _))
        .WillOnce(SetArgPointee<2>(GL_FALSE));
}

void ExpectProgramLinkSuccess(const GLint program, mtd::MockGL &mock_gl)
{
    EXPECT_CALL(mock_gl, glGetProgramiv(program, GL_LINK_STATUS, _))
        .WillOnce(SetArgPointee<2>(GL_TRUE));
}

void SetUpMockGraphicsProgram(mtd::MockGL &mock_gl, const std::function<void(const GLint, mtd::MockGL &)> &program_link_expectation)
{
    /* Graphics Program */
    EXPECT_CALL(mock_gl, glCreateProgram())
            .WillOnce(Return(stub_program));
    EXPECT_CALL(mock_gl, glAttachShader(stub_program, stub_v_shader));
    EXPECT_CALL(mock_gl, glAttachShader(stub_program, stub_f_shader));
    EXPECT_CALL(mock_gl, glLinkProgram(stub_program));
    program_link_expectation(stub_program, mock_gl);
}

void SetUpMockProgramData(mtd::MockGL &mock_gl)
{
    /* Uniforms and Attributes */
    EXPECT_CALL(mock_gl, glUseProgram(stub_program));

    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(screen_to_gl_coords_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(tex_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(transform_uniform_location));
    EXPECT_CALL(mock_gl, glGetUniformLocation(stub_program, _))
        .WillOnce(Return(alpha_uniform_location));
    EXPECT_CALL(mock_gl, glGetAttribLocation(stub_program, _))
        .WillOnce(Return(position_attr_location));
    EXPECT_CALL(mock_gl, glGetAttribLocation(stub_program, _))
        .WillOnce(Return(texcoord_attr_location));

    EXPECT_CALL(mock_gl, glUniformMatrix4fv(screen_to_gl_coords_uniform_location, 1, GL_FALSE, _));
}

void SetUpMockRenderTexture(mtd::MockGL &mock_gl)
{
    /* Set up the render texture */
    EXPECT_CALL(mock_gl, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(stub_texture));
    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    EXPECT_CALL(mock_gl, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
}

void FillMockVertexBuffer(mtd::MockGL &mock_gl)
{
    /* Set up VBO */
    EXPECT_CALL(mock_gl, glGenBuffers(1, _))
        .WillOnce(SetArgPointee<1>(stub_vbo));
    EXPECT_CALL(mock_gl, glBindBuffer(GL_ARRAY_BUFFER, stub_vbo));
    EXPECT_CALL(mock_gl, glBufferData(GL_ARRAY_BUFFER, _, _, GL_STATIC_DRAW));

    /* These should go away */
    EXPECT_CALL(mock_gl, glBindBuffer(GL_ARRAY_BUFFER, 0));
    EXPECT_CALL(mock_gl, glUseProgram(0));
}

class GLRendererSetupProcess :
    public testing::Test
{
public:

    mtd::MockGL mock_gl;
    mc::GLRendererFactory gl_renderer_factory;
    mir::geometry::Rectangle display_area;
};

ACTION_P2(CopyString, str, len)
{
    memcpy(arg3, str, len);
    arg3[len] = '\0';
}

ACTION_P(ReturnByConstReference, cref)
{
    return cref;
}

MATCHER_P(NthCharacterIsNul, n, "specified character is the nul-byte")
{
    return arg[n] == '\0';
}

TEST_F(GLRendererSetupProcess, vertex_shader_compiler_failure_recovers_and_throws)
{
    using namespace std::placeholders;

    SetUpMockVertexShader(mock_gl, std::bind(ExpectShaderCompileFailure, _1, _2));

    EXPECT_CALL(mock_gl, glGetShaderiv(stub_v_shader, GL_INFO_LOG_LENGTH, _))
        .WillOnce(SetArgPointee<2>(stub_info_log_length));
    EXPECT_CALL(mock_gl, glGetShaderInfoLog(stub_v_shader,
        stub_info_log_length,
        _,
        NthCharacterIsNul(stub_info_log_length)))
            .WillOnce(CopyString(stub_info_log.c_str(),
                stub_info_log.size()));

    EXPECT_THROW({
        auto r = gl_renderer_factory.create_renderer_for(display_area);
    }, std::runtime_error);
}

TEST_F(GLRendererSetupProcess, fragment_shader_compiler_failure_recovers_and_throw)
{
    using namespace std::placeholders;

    SetUpMockVertexShader(mock_gl, std::bind(ExpectShaderCompileSuccess, _1, _2));
    SetUpMockFragmentShader(mock_gl, std::bind(ExpectShaderCompileFailure, _1, _2));

    EXPECT_CALL(mock_gl, glGetShaderiv(stub_f_shader, GL_INFO_LOG_LENGTH, _))
        .WillOnce(SetArgPointee<2>(stub_info_log_length));
    EXPECT_CALL(mock_gl, glGetShaderInfoLog(stub_f_shader,
        stub_info_log_length,
        _,
        NthCharacterIsNul(stub_info_log_length)))
            .WillOnce(CopyString(stub_info_log.c_str(),
                stub_info_log.size()));

    EXPECT_THROW({
        auto r = gl_renderer_factory.create_renderer_for(display_area);
    }, std::runtime_error);
}

TEST_F(GLRendererSetupProcess, graphics_program_linker_failure_recovers_and_throw)
{
    using namespace std::placeholders;

    SetUpMockVertexShader(mock_gl, std::bind(ExpectShaderCompileSuccess, _1, _2));
    SetUpMockFragmentShader(mock_gl, std::bind(ExpectShaderCompileSuccess, _1, _2));
    SetUpMockGraphicsProgram(mock_gl, std::bind(ExpectProgramLinkFailure, _1, _2));

    EXPECT_CALL(mock_gl, glGetProgramiv(stub_program, GL_INFO_LOG_LENGTH, _))
            .WillOnce(SetArgPointee<2>(stub_info_log_length));
    EXPECT_CALL(mock_gl, glGetProgramInfoLog(stub_program,
        stub_info_log_length,
        _,
        NthCharacterIsNul(stub_info_log_length)))
            .WillOnce(CopyString(stub_info_log.c_str(),
                stub_info_log.size()));

    EXPECT_THROW({
        auto r = gl_renderer_factory.create_renderer_for(display_area);
    }, std::runtime_error);
}

class GLRenderer :
    public testing::Test
{
public:

    GLRenderer()
    {
        using namespace std::placeholders;

        InSequence s;

        SetUpMockVertexShader(mock_gl, std::bind(ExpectShaderCompileSuccess, _1, _2));
        SetUpMockFragmentShader(mock_gl, std::bind(ExpectShaderCompileSuccess, _1, _2));
        SetUpMockGraphicsProgram(mock_gl, std::bind(ExpectProgramLinkSuccess, _1,_2));
        SetUpMockProgramData(mock_gl);
        SetUpMockRenderTexture(mock_gl);
        EXPECT_CALL(mock_gl, glUniform1i(tex_uniform_location, 0));
        FillMockVertexBuffer(mock_gl);

        mc::GLRendererFactory gl_renderer_factory;
        renderer = gl_renderer_factory.create_renderer_for(display_area);
    }

    mtd::MockGL         mock_gl;
    mir::geometry::Rectangle display_area;
    std::unique_ptr<mc::Renderer> renderer;
    glm::mat4           trans;
};

}

TEST_F(GLRenderer, TestSetUpRenderContextBeforeRendering)
{
    using namespace std::placeholders;

    mtd::MockBufferStream stream;
    mtd::MockCompositingCriteria criteria;
    mtd::MockBuffer gr;

    int save_count = 0;
    std::vector<std::shared_ptr<void>> saved_resources;
    auto saving_lambda = [&] (std::shared_ptr<void> const& saved_resource)
    {
        save_count++;
        saved_resources.push_back(saved_resource);
    };

    InSequence seq;

    EXPECT_CALL(mock_gl, glUseProgram(stub_program));
    EXPECT_CALL(criteria, shaped())
        .WillOnce(Return(true));
    EXPECT_CALL(mock_gl, glEnable(GL_BLEND));
    EXPECT_CALL(mock_gl, glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    EXPECT_CALL(mock_gl, glActiveTexture(GL_TEXTURE0));

    EXPECT_CALL(criteria, transformation())
        .WillOnce(ReturnRef(trans));
    EXPECT_CALL(mock_gl, glUniformMatrix4fv(transform_uniform_location, 1, GL_FALSE, _));
    EXPECT_CALL(criteria, alpha())
        .WillOnce(Return(0.0f));
    EXPECT_CALL(mock_gl, glUniform1f(alpha_uniform_location, _));
    EXPECT_CALL(mock_gl, glBindBuffer(GL_ARRAY_BUFFER, stub_vbo));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(position_attr_location, 3, GL_FLOAT, GL_FALSE, _, _));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(texcoord_attr_location, 2, GL_FLOAT, GL_FALSE, _, _));

    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));

    EXPECT_CALL(stream, lock_compositor_buffer(_))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(gr)));
    EXPECT_CALL(gr, bind_to_texture());

    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(position_attr_location));
    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(texcoord_attr_location));

    EXPECT_CALL(mock_gl, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(texcoord_attr_location));
    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(position_attr_location));

    renderer->render(saving_lambda, criteria, stream);

    EXPECT_EQ(1, save_count);
    auto result = std::find(saved_resources.begin(), saved_resources.end(), mt::fake_shared(gr));
    EXPECT_NE(saved_resources.end(), result);
}

TEST_F(GLRenderer, disables_blending_for_rgbx_surfaces)
{
    using namespace std::placeholders;

    mtd::MockBufferStream stream;
    mtd::MockCompositingCriteria criteria;
    mtd::MockBuffer gr;

    int save_count = 0;
    std::vector<std::shared_ptr<void>> saved_resources;
    auto saving_lambda = [&] (std::shared_ptr<void> const& saved_resource)
    {
        save_count++;
        saved_resources.push_back(saved_resource);
    };

    InSequence seq;

    EXPECT_CALL(mock_gl, glUseProgram(stub_program));
    EXPECT_CALL(criteria, shaped())
        .WillOnce(Return(false));
    EXPECT_CALL(criteria, alpha())
        .WillOnce(Return(1.0f));
    EXPECT_CALL(mock_gl, glDisable(GL_BLEND));
    EXPECT_CALL(mock_gl, glActiveTexture(GL_TEXTURE0));

    EXPECT_CALL(criteria, transformation())
        .WillOnce(ReturnRef(trans));
    EXPECT_CALL(mock_gl, glUniformMatrix4fv(transform_uniform_location, 1, GL_FALSE, _));
    EXPECT_CALL(criteria, alpha())
        .WillOnce(Return(1.0f));
    EXPECT_CALL(mock_gl, glUniform1f(alpha_uniform_location, _));
    EXPECT_CALL(mock_gl, glBindBuffer(GL_ARRAY_BUFFER, stub_vbo));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(position_attr_location, 3, GL_FLOAT, GL_FALSE, _, _));
    EXPECT_CALL(mock_gl, glVertexAttribPointer(texcoord_attr_location, 2, GL_FLOAT, GL_FALSE, _, _));

    EXPECT_CALL(mock_gl, glBindTexture(GL_TEXTURE_2D, stub_texture));

    EXPECT_CALL(stream, lock_compositor_buffer(_))
        .Times(1)
        .WillOnce(Return(mt::fake_shared(gr)));
    EXPECT_CALL(gr, bind_to_texture());

    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(position_attr_location));
    EXPECT_CALL(mock_gl, glEnableVertexAttribArray(texcoord_attr_location));

    EXPECT_CALL(mock_gl, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(texcoord_attr_location));
    EXPECT_CALL(mock_gl, glDisableVertexAttribArray(position_attr_location));

    renderer->render(saving_lambda, criteria, stream);

    EXPECT_EQ(1, save_count);
    auto result = std::find(saved_resources.begin(), saved_resources.end(), mt::fake_shared(gr));
    EXPECT_NE(saved_resources.end(), result);
}
