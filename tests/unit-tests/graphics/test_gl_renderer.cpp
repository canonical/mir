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
#include <mir/geometry/size.h>
#include <mir/graphics/gl_renderer.h>
#include <mir_test_doubles/mock_renderable.h>
#include <mir_test_doubles/mock_graphic_region.h>
#include <mir/graphics/renderable.h>
#include "mir/surfaces/graphic_region.h"
#include <mir/surfaces/buffer_bundle.h>
#include <mir_test/gl_mock.h>

using testing::SetArgPointee;
using testing::InSequence;
using testing::Return;
using testing::_;

namespace mtd=mir::test::doubles;
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
const std::string stub_info_log = "something failed!";
const size_t stub_info_log_length = stub_info_log.size();
const size_t stub_info_log_buffer_length = stub_info_log_length + 1;

void ExpectShaderCompileFailure(const GLint shader, mir::GLMock &gl_mock)
{
    EXPECT_CALL(gl_mock, glGetShaderiv(shader, GL_COMPILE_STATUS, _))
        .WillOnce(SetArgPointee<2>(GL_FALSE));
}

void ExpectShaderCompileSuccess(const GLint shader, mir::GLMock &gl_mock)
{
    EXPECT_CALL(gl_mock, glGetShaderiv(shader, GL_COMPILE_STATUS, _))
        .WillOnce(SetArgPointee<2>(GL_TRUE));
}

void SetUpMockVertexShader(mir::GLMock &gl_mock, const std::function<void(const GLint, mir::GLMock &)> &shader_compile_expectation)
{
    /* Vertex Shader */
    EXPECT_CALL(gl_mock, glCreateShader(GL_VERTEX_SHADER))
        .WillOnce(Return(stub_v_shader));
    EXPECT_CALL(gl_mock, glShaderSource(stub_v_shader, 1, _, 0));
    EXPECT_CALL(gl_mock, glCompileShader(stub_v_shader));
    shader_compile_expectation(stub_v_shader, gl_mock);
}

void SetUpMockFragmentShader(mir::GLMock &gl_mock, const std::function<void(const GLint, mir::GLMock &)> &shader_compile_expectation)
{
    /* Fragment Shader */
    EXPECT_CALL(gl_mock, glCreateShader(GL_FRAGMENT_SHADER))
        .WillOnce(Return(stub_f_shader));
    EXPECT_CALL(gl_mock, glShaderSource(stub_f_shader, 1, _, 0));
    EXPECT_CALL(gl_mock, glCompileShader(stub_f_shader));
    shader_compile_expectation(stub_f_shader, gl_mock);
}

void ExpectProgramLinkFailure(const GLint program, mir::GLMock &gl_mock)
{
    EXPECT_CALL(gl_mock, glGetProgramiv(program, GL_LINK_STATUS, _))
        .WillOnce(SetArgPointee<2>(GL_FALSE));
}

void ExpectProgramLinkSuccess(const GLint program, mir::GLMock &gl_mock)
{
    EXPECT_CALL(gl_mock, glGetProgramiv(program, GL_LINK_STATUS, _))
        .WillOnce(SetArgPointee<2>(GL_TRUE));
}

void SetUpMockGraphicsProgram(mir::GLMock &gl_mock, const std::function<void(const GLint, mir::GLMock &)> &program_link_expectation)
{
    /* Graphics Program */
    EXPECT_CALL(gl_mock, glCreateProgram())
            .WillOnce(Return(stub_program));
    EXPECT_CALL(gl_mock, glAttachShader(stub_program, stub_v_shader));
    EXPECT_CALL(gl_mock, glAttachShader(stub_program, stub_f_shader));
    EXPECT_CALL(gl_mock, glLinkProgram(stub_program));
    program_link_expectation(stub_program, gl_mock);
}

void SetUpMockProgramData(mir::GLMock &gl_mock)
{
    /* Uniforms and Attributes */
    EXPECT_CALL(gl_mock, glUseProgram(stub_program));

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
}

void SetUpMockRenderTexture(mir::GLMock &gl_mock)
{
    /* Set up the render texture */
    EXPECT_CALL(gl_mock, glGenTextures(1, _))
        .WillOnce(SetArgPointee<1>(stub_texture));
    EXPECT_CALL(gl_mock, glBindTexture(GL_TEXTURE_2D, stub_texture));
    EXPECT_CALL(gl_mock, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    EXPECT_CALL(gl_mock, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    EXPECT_CALL(gl_mock, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    EXPECT_CALL(gl_mock, glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
}

void FillMockVertexBuffer(mir::GLMock &gl_mock)
{
    /* Set up VBO */
    EXPECT_CALL(gl_mock, glGenBuffers(1, _))
        .WillOnce(SetArgPointee<1>(stub_vbo));
    EXPECT_CALL(gl_mock, glBindBuffer(GL_ARRAY_BUFFER, stub_vbo));
    EXPECT_CALL(gl_mock, glBufferData(GL_ARRAY_BUFFER, _, _, GL_STATIC_DRAW));

    /* These should go away */
    EXPECT_CALL(gl_mock, glBindBuffer(GL_ARRAY_BUFFER, 0));
    EXPECT_CALL(gl_mock, glUseProgram(0));
}

class GLRendererSetupProcess :
    public testing::Test
{
public:

    mir::GLMock gl_mock;
    mir::geometry::Size display_size;
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

    SetUpMockVertexShader(gl_mock, std::bind(ExpectShaderCompileFailure, _1, _2));

    EXPECT_CALL(gl_mock, glGetShaderiv(stub_v_shader, GL_INFO_LOG_LENGTH, _))
        .WillOnce(SetArgPointee<2>(stub_info_log_length));
    EXPECT_CALL(gl_mock, glGetShaderInfoLog(stub_v_shader,
        stub_info_log_length,
        _,
        NthCharacterIsNul(stub_info_log_length)))
            .WillOnce(CopyString(stub_info_log.c_str(),
                stub_info_log.size()));

    EXPECT_THROW({
    std::unique_ptr<mg::GLRenderer> r;
    r.reset(new mg::GLRenderer(display_size));
    }, std::runtime_error);
}

TEST_F(GLRendererSetupProcess, fragment_shader_compiler_failure_recovers_and_throw)
{
    using namespace std::placeholders;

    SetUpMockVertexShader(gl_mock, std::bind(ExpectShaderCompileSuccess, _1, _2));
    SetUpMockFragmentShader(gl_mock, std::bind(ExpectShaderCompileFailure, _1, _2));

    EXPECT_CALL(gl_mock, glGetShaderiv(stub_f_shader, GL_INFO_LOG_LENGTH, _))
        .WillOnce(SetArgPointee<2>(stub_info_log_length));
    EXPECT_CALL(gl_mock, glGetShaderInfoLog(stub_f_shader,
        stub_info_log_length,
        _,
        NthCharacterIsNul(stub_info_log_length)))
            .WillOnce(CopyString(stub_info_log.c_str(),
                stub_info_log.size()));

    EXPECT_THROW({
        std::unique_ptr<mg::GLRenderer> r;
        r.reset(new mg::GLRenderer(display_size));
    }, std::runtime_error);
}

TEST_F(GLRendererSetupProcess, graphics_program_linker_failure_recovers_and_throw)
{
    using namespace std::placeholders;

    SetUpMockVertexShader(gl_mock, std::bind(ExpectShaderCompileSuccess, _1, _2));
    SetUpMockFragmentShader(gl_mock, std::bind(ExpectShaderCompileSuccess, _1, _2));
    SetUpMockGraphicsProgram(gl_mock, std::bind(ExpectProgramLinkFailure, _1, _2));

    EXPECT_CALL(gl_mock, glGetProgramiv(stub_program, GL_INFO_LOG_LENGTH, _))
            .WillOnce(SetArgPointee<2>(stub_info_log_length));
    EXPECT_CALL(gl_mock, glGetProgramInfoLog(stub_program,
        stub_info_log_length,
        _,
        NthCharacterIsNul(stub_info_log_length)))
            .WillOnce(CopyString(stub_info_log.c_str(),
                stub_info_log.size()));

    EXPECT_THROW({
        std::unique_ptr<mg::GLRenderer> r;
        r.reset(new mg::GLRenderer(display_size));
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

        SetUpMockVertexShader(gl_mock, std::bind(ExpectShaderCompileSuccess, _1, _2));
        SetUpMockFragmentShader(gl_mock, std::bind(ExpectShaderCompileSuccess, _1, _2));
        SetUpMockGraphicsProgram(gl_mock, std::bind(ExpectProgramLinkSuccess, _1,_2));
        SetUpMockProgramData(gl_mock);
        SetUpMockRenderTexture(gl_mock);
        EXPECT_CALL(gl_mock, glUniform1i(tex_uniform_location, 0));
        FillMockVertexBuffer(gl_mock);

        renderer.reset(new mg::GLRenderer(display_size));
    }

    mir::GLMock         gl_mock;
    mir::geometry::Size display_size;
    std::unique_ptr<mg::GLRenderer> renderer;
};

void NullGraphicRegionDeleter(mtd::MockGraphicRegion * /* gr */)
{
}

}

TEST_F(GLRenderer, TestSetUpRenderContextBeforeRenderingRenderable)
{
    using namespace std::placeholders;

    mtd::MockRenderable rd;
    mtd::MockGraphicRegion gr;
    std::shared_ptr<mtd::MockGraphicRegion> gr_ptr(&gr, std::bind(NullGraphicRegionDeleter, _1));

    int save_count = 0;
    std::vector<std::shared_ptr<void>> saved_resources;
    auto saving_lambda = [&] (std::shared_ptr<void> const& saved_resource)
    {
        save_count++;
        saved_resources.push_back(saved_resource);
    };

    mir::geometry::Point tl;
    mir::geometry::Size  s;
    glm::mat4            transformation;

    tl.x = mir::geometry::X(1);
    tl.y = mir::geometry::Y(2);

    s.width = mir::geometry::Width(10);
    s.height = mir::geometry::Height(20);

    InSequence seq;

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

    EXPECT_CALL(rd, graphic_region())
        .Times(1)
        .WillOnce(Return(gr_ptr));
    EXPECT_CALL(gr, bind_to_texture());

    EXPECT_CALL(gl_mock, glEnableVertexAttribArray(position_attr_location));
    EXPECT_CALL(gl_mock, glEnableVertexAttribArray(texcoord_attr_location));

    EXPECT_CALL(gl_mock, glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    EXPECT_CALL(gl_mock, glDisableVertexAttribArray(texcoord_attr_location));
    EXPECT_CALL(gl_mock, glDisableVertexAttribArray(position_attr_location));

    renderer->render(saving_lambda, rd);

    EXPECT_EQ(1, save_count);
    auto result = std::find(saved_resources.begin(), saved_resources.end(), gr_ptr);
    EXPECT_NE(saved_resources.end(), result);
}

TEST_F(GLRenderer, TestRenderEnsureNoBind)
{
    using namespace std::placeholders;

    mtd::MockRenderable rd;
    EXPECT_CALL(gl_mock, glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,_));

    renderer->ensure_no_live_buffers_bound();
}
