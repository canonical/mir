/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/gl/program.h"
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mgl = mir::gl;

namespace
{
typedef void(*MirGLGetObjectInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void(*MirGLGetObjectiv)(GLuint, GLenum, GLint *);

void GetObjectLogAndThrow(MirGLGetObjectInfoLog getObjectInfoLog,
                          MirGLGetObjectiv      getObjectiv,
                          std::string const &   msg,
                          GLuint                object)
{
    GLint object_log_length = 0;
    (*getObjectiv)(object, GL_INFO_LOG_LENGTH, &object_log_length);

    const GLuint object_log_buffer_length = object_log_length + 1;
    std::string  object_info_log;

    object_info_log.resize(object_log_buffer_length);
    (*getObjectInfoLog)(object, object_log_length, NULL,
                        const_cast<GLchar *>(object_info_log.data()));

    std::string object_info_err(msg + "\n");
    object_info_err += object_info_log;

    BOOST_THROW_EXCEPTION(std::runtime_error(object_info_err));
}
}

mgl::Shader::Shader(GLchar const* shader_src, GLuint type)
    : shader(glCreateShader(type))
{
    GLint param{0};
    glShaderSource(shader, 1, &shader_src, 0);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
    if (param == GL_FALSE)
    {
        glDeleteShader(shader);
        GetObjectLogAndThrow(glGetShaderInfoLog,
            glGetShaderiv,
            "Failed to compile shader:",
            shader);
    }
}

mgl::Shader::~Shader()
{
    glDeleteShader(shader);
}

mgl::Shader::operator GLuint() const
{
    return shader;
}

mgl::SimpleProgram::SimpleProgram(
    GLchar const* vertex_shader_src,
    GLchar const* fragment_shader_src)
  : vertex_shader(vertex_shader_src, GL_VERTEX_SHADER),
    fragment_shader(fragment_shader_src, GL_FRAGMENT_SHADER),
    program(glCreateProgram())
{
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    GLint param = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &param);
    if (param == GL_FALSE)
    {
        glDeleteProgram(program);
        GetObjectLogAndThrow(glGetProgramInfoLog,
            glGetProgramiv,
            "Failed to link program:",
            program);
    }
}

mgl::SimpleProgram::~SimpleProgram()
{
    glDeleteProgram(program);
}

mgl::SimpleProgram::operator GLuint() const
{
    return program;
}
