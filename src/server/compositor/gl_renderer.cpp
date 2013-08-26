/* Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gl_renderer.h"
#include "mir/compositor/compositing_criteria.h"
#include "mir/surfaces/buffer_stream.h"
#include "mir/graphics/buffer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{

const GLchar* vertex_shader_src =
{
    "attribute vec3 position;\n"
    "attribute vec2 texcoord;\n"
    "uniform mat4 screen_to_gl_coords;\n"
    "uniform mat4 transform;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   gl_Position = screen_to_gl_coords * transform * vec4(position, 1.0);\n"
    "   v_texcoord = texcoord;\n"
    "}\n"
};

const GLchar* fragment_shader_src =
{
    "precision mediump float;\n"
    "uniform sampler2D tex;\n"
    "uniform float alpha;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   vec4 frag = texture2D(tex, v_texcoord);\n"
    "   gl_FragColor = vec4(frag.xyz, frag.a * alpha);\n"
    "}\n"
};

struct VertexAttributes
{
    glm::vec3 position;
    glm::vec2 texcoord;
};

/*
 * The texture coordinates are y-inverted to account for the difference in the
 * texture and renderable pixel data row order. In particular, GL textures
 * expect pixel data in rows starting from the bottom and moving up the image,
 * whereas our renderables provide data in rows starting from the top and
 * moving down the image.
 */
VertexAttributes vertex_attribs[4] =
{
    {
        glm::vec3{-0.5f, -0.5f, 0.0f},
        glm::vec2{0.0f, 0.0f}
    },
    {
        glm::vec3{-0.5f, 0.5f, 0.0f},
        glm::vec2{0.0f, 1.0f},
    },
    {
        glm::vec3{0.5f, -0.5f, 0.0f},
        glm::vec2{1.0f, 0.0f},
    },
    {
        glm::vec3{0.5f, 0.5f, 0.0f},
        glm::vec2{1.0f, 1.0f}
    }
};

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

mc::GLRenderer::Resources::Resources() :
    vertex_shader(0),
    fragment_shader(0),
    program(0),
    position_attr_loc(0),
    texcoord_attr_loc(0),
    transform_uniform_loc(0),
    alpha_uniform_loc(0),
    vertex_attribs_vbo(0),
    texture(0)
{
}

mc::GLRenderer::Resources::~Resources()
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

void mc::GLRenderer::Resources::setup(geometry::Rectangle const& display_area)
{
    GLint param = 0;

    /* Create shaders and program */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, 0);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &param);
    if (param == GL_FALSE)
    {
        GetObjectLogAndThrow(glGetShaderInfoLog,
            glGetShaderiv,
            "Failed to compile vertex shader:",
            vertex_shader);
    }

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, 0);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &param);
    if (param == GL_FALSE)
    {
        GetObjectLogAndThrow(glGetShaderInfoLog,
            glGetShaderiv,
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
        GetObjectLogAndThrow(glGetProgramInfoLog,
            glGetProgramiv,
            "Failed to link program:",
            program);
    }

    glUseProgram(program);

    /* Set up program variables */
    GLint mat_loc = glGetUniformLocation(program, "screen_to_gl_coords");
    GLint tex_loc = glGetUniformLocation(program, "tex");
    transform_uniform_loc = glGetUniformLocation(program, "transform");
    alpha_uniform_loc = glGetUniformLocation(program, "alpha");
    position_attr_loc = glGetAttribLocation(program, "position");
    texcoord_attr_loc = glGetAttribLocation(program, "texcoord");

    /*
     * Create and set screen_to_gl_coords transformation matrix.
     * The screen_to_gl_coords matrix transforms from the screen coordinate system
     * (top-left is (0,0), bottom-right is (W,H)) to the normalized GL coordinate system
     * (top-left is (-1,1), bottom-right is (1,-1))
     */
    glm::mat4 screen_to_gl_coords = glm::translate(glm::mat4{1.0f}, glm::vec3{-1.0f, 1.0f, 0.0f});
    screen_to_gl_coords = glm::scale(screen_to_gl_coords,
            glm::vec3{2.0f / display_area.size.width.as_float(),
                      -2.0f / display_area.size.height.as_float(),
                      1.0f});
    screen_to_gl_coords = glm::translate(screen_to_gl_coords,
            glm::vec3{-display_area.top_left.x.as_float(),
                      -display_area.top_left.y.as_float(),
                      0.0f});

    glUniformMatrix4fv(mat_loc, 1, GL_FALSE, glm::value_ptr(screen_to_gl_coords));

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(tex_loc, 0);

    /* Create VBO */
    glGenBuffers(1, &vertex_attribs_vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_attribs_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_attribs),
            glm::value_ptr(vertex_attribs[0].position), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

mc::GLRenderer::GLRenderer(geom::Rectangle const& display_area)
    : frameno{0}
{
    resources.setup(display_area);
}

void mc::GLRenderer::render(
    std::function<void(std::shared_ptr<void> const&)> save_resource,
    CompositingCriteria const& criteria,
    mir::surfaces::BufferStream& stream)
{
    glUseProgram(resources.program);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    glUniformMatrix4fv(resources.transform_uniform_loc, 1, GL_FALSE,
                       glm::value_ptr(criteria.transformation()));
    glUniform1f(resources.alpha_uniform_loc, criteria.alpha());

    /* Set up vertex attribute data */
    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_attribs_vbo);
    glVertexAttribPointer(resources.position_attr_loc, 3, GL_FLOAT,
                          GL_FALSE, sizeof(VertexAttributes), 0);
    glVertexAttribPointer(resources.texcoord_attr_loc, 2, GL_FLOAT,
                          GL_FALSE, sizeof(VertexAttributes),
                          reinterpret_cast<void*>(sizeof(glm::vec3)));

    /* Use the renderable's texture */
    glBindTexture(GL_TEXTURE_2D, resources.texture);

    auto region_resource = stream.lock_compositor_buffer(frameno);
    region_resource->bind_to_texture();
    save_resource(region_resource);

    /* Draw */
    glEnableVertexAttribArray(resources.position_attr_loc);
    glEnableVertexAttribArray(resources.texcoord_attr_loc);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(resources.texcoord_attr_loc);
    glDisableVertexAttribArray(resources.position_attr_loc);
}

void mc::GLRenderer::clear(unsigned long frame)
{
    frameno = frame;
    glClear(GL_COLOR_BUFFER_BIT);
}

