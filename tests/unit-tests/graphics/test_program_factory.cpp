/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#include <functional>
#include <string>
#include <cstring>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <mir/geometry/rectangle.h>
#include "src/server/graphics/program_factory.h"
#include <mir/test/fake_shared.h>
#include <mir/test/doubles/mock_buffer.h>
#include <mir/test/doubles/mock_renderable.h>
#include <mir/test/doubles/mock_buffer_stream.h>
#include <mir/compositor/buffer_stream.h>
#include <mir/test/doubles/mock_gl.h>

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
        .WillRepeatedly(SetArgPointee<2>(GL_TRUE));
}

void SetUpMockVertexShader(mtd::MockGL &mock_gl, const std::function<void(const GLint, mtd::MockGL &)> &shader_compile_expectation)
{
    /* Vertex Shader */
    ON_CALL(mock_gl, glCreateShader(GL_VERTEX_SHADER))
        .WillByDefault(Return(stub_v_shader));
    shader_compile_expectation(stub_v_shader, mock_gl);
}

void SetUpMockFragmentShader(mtd::MockGL &mock_gl, const std::function<void(const GLint, mtd::MockGL &)> &shader_compile_expectation)
{
    /* Fragment Shader */
    ON_CALL(mock_gl, glCreateShader(GL_FRAGMENT_SHADER))
        .WillByDefault(Return(stub_f_shader));
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
        .WillRepeatedly(SetArgPointee<2>(GL_TRUE));
}

void SetUpMockGraphicsProgram(mtd::MockGL &mock_gl, const std::function<void(const GLint, mtd::MockGL &)> &program_link_expectation)
{
    /* Graphics Program */
    ON_CALL(mock_gl, glCreateProgram())
        .WillByDefault(Return(stub_program));
    program_link_expectation(stub_program, mock_gl);
}

class ProgramFactory : public testing::Test
{
public:
    testing::NiceMock<mtd::MockGL> mock_gl;
    mg::ProgramFactory factory;
};

ACTION_P2(CopyString, str, len)
{
    memcpy(arg3, str, len);
    arg3[len] = '\0';
}

MATCHER_P(NthCharacterIsNul, n, "specified character is the nul-byte")
{
    return arg[n] == '\0';
}

TEST_F(ProgramFactory, vertex_shader_compiler_failure_recovers_and_throws)
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
        auto p = factory.create_gl_program("foo", "bar");
    }, std::runtime_error);
}

TEST_F(ProgramFactory, fragment_shader_compiler_failure_recovers_and_throw)
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
        auto p = factory.create_gl_program("foo", "bar");
    }, std::runtime_error);
}

TEST_F(ProgramFactory, graphics_program_linker_failure_recovers_and_throw)
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
        auto p = factory.create_gl_program("foo", "bar");
    }, std::runtime_error);
}

TEST_F(ProgramFactory, graphics_program_creation_success)
{
    using namespace std::placeholders;

    SetUpMockVertexShader(mock_gl, std::bind(ExpectShaderCompileSuccess, _1, _2));
    SetUpMockFragmentShader(mock_gl, std::bind(ExpectShaderCompileSuccess, _1, _2));
    SetUpMockGraphicsProgram(mock_gl, std::bind(ExpectProgramLinkSuccess, _1, _2));

    auto p = factory.create_gl_program("foo", "bar");
}
}
