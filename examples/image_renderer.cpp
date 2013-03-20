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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "image_renderer.h"

// Unfortunately we have to ignore warnings/errors in 3rd party code.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wall"
#include <glm/glm.hpp>
#pragma GCC diagnostic pop
#include <glm/gtc/type_ptr.hpp>

#include <memory>
#include <vector>

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mt = mir::tools;

namespace
{

const GLchar* vertex_shader_src =
{
    "attribute vec3 position;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   gl_Position = vec4(position, 1.0);\n"
    "   v_texcoord = position.xy * 0.5 + 0.5;\n"
    "}\n"
};

const GLchar* fragment_shader_src =
{
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   gl_FragColor = texture2D(tex, v_texcoord);\n"
    "}\n"
};


glm::vec3 vertex_attribs[4] =
{
    glm::vec3{-1.0f, 1.0f, 0.0f},
    glm::vec3{-1.0f, -1.0f, 0.0f},
    glm::vec3{1.0f, 1.0f, 0.0f},
    glm::vec3{1.0f, -1.0f, 0.0f}
};

typedef void(*MirGLGetObjectInfoLog)(GLuint, GLsizei, GLsizei *, GLchar *);
typedef void(*MirGLGetObjectiv)(GLuint, GLenum, GLint *);

void throw_with_object_log(MirGLGetObjectInfoLog getObjectInfoLog,
                           MirGLGetObjectiv      getObjectiv,
                           std::string const &   msg,
                           GLuint                object)
{
    GLint object_log_length = 0;
    (*getObjectiv)(object, GL_INFO_LOG_LENGTH, &object_log_length);

    GLuint const object_log_buffer_length = object_log_length + 1;
    std::vector<char> log_chars(object_log_buffer_length);

    (*getObjectInfoLog)(object, object_log_buffer_length, NULL, log_chars.data());

    std::string object_info_err(msg + "\n");
    object_info_err.append(log_chars.begin(), log_chars.end() - 1);

    BOOST_THROW_EXCEPTION(std::runtime_error(object_info_err));
}

}

mt::ImageRenderer::ImageRenderer(const uint8_t* pixel_data, mir::geometry::Size size,
                                 uint32_t bytes_per_pixel)
{
    resources.setup();

    /* Upload the texture */
    glBindTexture(GL_TEXTURE_2D, resources.texture);

    GLenum format = (bytes_per_pixel == 3) ? GL_RGB : GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format,
                 size.width.as_uint32_t(), size.height.as_uint32_t(), 0,
                 format, GL_UNSIGNED_BYTE, pixel_data);
}

mt::ImageRenderer::Resources::~Resources()
{
    if (vertex_shader)
        glDeleteShader(vertex_shader);
    if (fragment_shader)
        glDeleteShader(fragment_shader);
    if (program)
        glDeleteProgram(program);
    if (vertex_attribs_vbo)
        glDeleteBuffers(1, &vertex_attribs_vbo);
    if (texture)
        glDeleteTextures(1, &texture);
}

void mt::ImageRenderer::Resources::setup()
{
    GLint param = 0;

    /* Create shaders and program */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, 0);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &param);
    if (param == GL_FALSE)
    {
        throw_with_object_log(glGetShaderInfoLog, glGetShaderiv,
                              "Failed to compile vertex shader:",
                              vertex_shader);
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, 0);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &param);
    if (param == GL_FALSE)
    {
        throw_with_object_log(glGetShaderInfoLog, glGetShaderiv,
                              "Failed to compile fragment shader:",
                              fragment_shader);
    }

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &param);
    if (param == GL_FALSE)
    {
        throw_with_object_log(glGetProgramInfoLog, glGetProgramiv,
                              "Failed to link shader program:",
                              program);
    }

    glUseProgram(program);

    /* Set up program variables */
    GLint tex_loc = glGetUniformLocation(program, "tex");
    position_attr_loc = glGetAttribLocation(program, "position");

    /* Create the texture */
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(tex_loc, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    /* Create VBO */
    glGenBuffers(1, &vertex_attribs_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_attribs_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_attribs),
                 glm::value_ptr(vertex_attribs[0]), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}


void mt::ImageRenderer::render()
{
    glUseProgram(resources.program);

    glActiveTexture(GL_TEXTURE0);

    /* Set up vertex attribute data */
    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_attribs_vbo);
    glVertexAttribPointer(resources.position_attr_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindTexture(GL_TEXTURE_2D, resources.texture);

    /* Draw */
    glEnableVertexAttribArray(resources.position_attr_loc);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(resources.position_attr_loc);
}
