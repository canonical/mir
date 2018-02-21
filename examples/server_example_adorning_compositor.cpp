/*
 * Copyright © 2015, 2016 Canonical Ltd.
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
 *
 * Authored By: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "server_example_adorning_compositor.h"
#include "as_render_target.h"

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/buffer.h"
#include "mir/compositor/compositor_report.h"
#include "mir/compositor/scene_element.h"
#include "mir/renderer/gl/texture_source.h"
#include "mir/renderer/gl/render_target.h"

#include <stdexcept>
#include <boost/throw_exception.hpp>

#include MIR_SERVER_GL_H

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
    std::tuple<float, float, float> const& background_rgb,
    std::shared_ptr<mc::CompositorReport> const& report) :
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
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
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
    program(vertex,  fragment),
    report(report)
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

namespace
{
bool renderable_is_occluded(
    mg::Renderable const& renderable,
    mir::geometry::Rectangle const& area,
    std::vector<mir::geometry::Rectangle>& coverage)
{
    static glm::mat4 const identity{};
    static mir::geometry::Rectangle const empty{};

    if (renderable.transformation() != identity)
        return false;  // Weirdly transformed. Assume never occluded.

    auto const& window = renderable.screen_position();
    auto const& clipped_window = window.intersection_with(area);

    if (clipped_window == empty)
        return true;  // Not in the area; definitely occluded.

    bool occluded = false;
    for (auto const& r : coverage)
    {
        if (r.contains(clipped_window))
        {
            occluded = true;
            break;
        }
    }

    if (!occluded && renderable.alpha() == 1.0f && !renderable.shaped())
        coverage.push_back(clipped_window);

    return occluded;
}

void remove_occlusions_from(mc::SceneElementSequence& elements, mir::geometry::Rectangle const& area)
{
    std::vector<mir::geometry::Rectangle> coverage;

    auto it = elements.rbegin();
    while (it != elements.rend())
    {
        auto const renderable = (*it)->renderable();
        if (renderable_is_occluded(*renderable, area, coverage))
        {
            (*it)->occluded();
            it = mc::SceneElementSequence::reverse_iterator(elements.erase(std::prev(it.base())));
        }
        else
        {
            it++;
        }
    }
}
}

void me::AdorningDisplayBufferCompositor::composite(compositor::SceneElementSequence&& scene_sequence)
{
    report->began_frame(this);

    remove_occlusions_from(scene_sequence, db.view_area());

    //note: If what should be drawn is expressible as a SceneElementSequence,
    //      mg::DisplayBuffer::overlay() should be used,
    //      to give the the display hardware a chance at an optimized render of
    //      the scene. In this example though, we want some custom elements, so
    //      we'll always use GLES.
    render_target->make_current();

    auto display_width  = db.view_area().size.width.as_int();
    auto display_height = db.view_area().size.height.as_int();

    glUseProgram(program.program);
    glViewport(0, 0, display_width, display_height);
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );

    mg::RenderableList renderable_list;
    renderable_list.reserve(scene_sequence.size());

    for(auto& element : scene_sequence)
    {
        //courteously inform the client that its rendered
        //if something is not to be rendered, mc::SceneElementSequence::occluded() should be called 
        element->rendered();

        auto const renderable = element->renderable();
        renderable_list.push_back(renderable);

        float width  = renderable->screen_position().size.width.as_int();
        float height = renderable->screen_position().size.height.as_int();
        float x = renderable->screen_position().top_left.x.as_int() - db.view_area().top_left.x.as_int();
        float y = renderable->screen_position().top_left.y.as_int() - db.view_area().top_left.y.as_int();
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

    report->renderables_in_frame(this, renderable_list);
    report->rendered_frame(this);

    render_target->swap_buffers();

    report->finished_frame(this);
}
