/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "server_example_adorning_compositor.h"
#include "as_render_target.h"

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/buffer.h"
#include "mir/compositor/scene_element.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir/renderer/gl/render_target.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

#include <GLES2/gl2.h>

namespace me = mir::examples;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mrg = mir::renderer::gl;

bool make_current(mrg::RenderTarget* render_target)
{
    render_target->make_current();
    return true;
}

me::AdorningDisplayBufferCompositor::Shader::Shader(GLchar const* const* src, GLuint type) :
    shader(glCreateShader(type))
{
    glShaderSource(shader, 1, src, 0);
    glCompileShader(shader);
}

me::AdorningDisplayBufferCompositor::Shader::~Shader()
{
    glDeleteShader(shader);
}

me::AdorningDisplayBufferCompositor::Program::Program(Shader& vertex, Shader& fragment) :
    program(glCreateProgram())
{
    glAttachShader(program, vertex.shader);
    glAttachShader(program, fragment.shader);
    glLinkProgram(program);
}

me::AdorningDisplayBufferCompositor::Program::~Program()
{
    glDeleteProgram(program);
}

me::AdorningDisplayBufferCompositor::AdorningDisplayBufferCompositor(
    mg::DisplayBuffer& display_buffer,
    std::tuple<float, float, float> const& background_rgb) :
    db{display_buffer},
    render_target{me::as_render_target(display_buffer)},
    vert_shader_src{
        "attribute vec4 vPosition;"
        "uniform vec2 pos;"
        "uniform vec2 scale;"
        "attribute vec2 uvCoord;"
        "varying vec2 texcoord;"
        "void main() {"
        "   gl_Position = vec4(vPosition.xy * scale + pos, 0.0, 1.0);"
        "   texcoord = uvCoord.xy;"
        "}"
    },
    frag_shader_src{
        "precision mediump float;"
        "varying vec2 texcoord;"
        "uniform sampler2D tex;"
        "uniform float alpha;"
        "void main() {"
        "   gl_FragColor = texture2D(tex, texcoord) * alpha;"
        "}"
    },
    current(make_current(render_target)),
    vertex(&vert_shader_src, GL_VERTEX_SHADER),
    fragment(&frag_shader_src, GL_FRAGMENT_SHADER),
    program(vertex,  fragment)
{
    glUseProgram(program.program);
    vPositionAttr = glGetAttribLocation(program.program, "vPosition");
    glVertexAttribPointer(vPositionAttr, 4, GL_FLOAT, GL_FALSE, 0, vertex_data);
    uvCoord = glGetAttribLocation(program.program, "uvCoord");
    glVertexAttribPointer(uvCoord, 2, GL_FLOAT, GL_FALSE, 0, uv_data);
    posUniform = glGetUniformLocation(program.program, "pos");
    glClearColor(std::get<0>(background_rgb), std::get<1>(background_rgb), std::get<2>(background_rgb), 1.0);
    scaleUniform = glGetUniformLocation(program.program, "scale");
    alphaUniform = glGetUniformLocation(program.program, "alpha");

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void me::AdorningDisplayBufferCompositor::composite(compositor::SceneElementSequence&& scene_sequence)
{
    //note: If what should be drawn is expressible as a SceneElementSequence,
    //      mg::DisplayBuffer::post_renderables_if_optimizable() should be used,
    //      to give the the display hardware a chance at an optimized render of
    //      the scene. In this example though, we want some custom elements, so
    //      we'll always use GLES.
    render_target->make_current();

    auto display_width  = db.view_area().size.width.as_float();
    auto display_height = db.view_area().size.height.as_float();

    glUseProgram(program.program);
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
    
    for(auto& element : scene_sequence)
    {
        //courteously inform the client that its rendered
        //if something is not to be rendered, mc::SceneElementSequence::occluded() should be called 
        element->rendered();

        auto const renderable = element->renderable();
        float width  = renderable->screen_position().size.width.as_float();
        float height = renderable->screen_position().size.height.as_float();
        float x = renderable->screen_position().top_left.x.as_float() - db.view_area().top_left.x.as_float();
        float y = renderable->screen_position().top_left.y.as_float() - db.view_area().top_left.y.as_float();
        float scale[2] {
            width/display_width * 2,
            height/display_height * -2};
        float position[2] {
            (x / display_width * 2.0f) - 1.0f,
            1.0f - (y / display_height * 2.0f)
        };
        float const plane_alpha = renderable->alpha();
        if (renderable->shaped() || plane_alpha < 1.0)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);

        glUniform2fv(posUniform, 1, position);
        glUniform2fv(scaleUniform, 1, scale);
        glUniform1fv(alphaUniform, 1, &plane_alpha);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        auto const texture_source =
            dynamic_cast<mir::renderer::gl::TextureSource*>(
                renderable->buffer()->native_buffer_base());
        if (!texture_source)
            BOOST_THROW_EXCEPTION(std::logic_error("Buffer does not support GL rendering"));
        texture_source->gl_bind_to_texture();

        glEnableVertexAttribArray(vPositionAttr);
        glEnableVertexAttribArray(uvCoord);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableVertexAttribArray(uvCoord);
        glDisableVertexAttribArray(vPositionAttr);
    }

    render_target->swap_buffers();
}
