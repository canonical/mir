/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gl_output_filter.h"
#include "gl_program_factory.h"

#include <sstream>

namespace mrc = mir::renderer::common;
namespace geom = mir::geometry;

namespace
{
// Shader that converts colors to grayscale.
GLchar const* const grayscale_src =
    "uniform sampler2D tex;\n"
    "vec4 sample_to_rgba(in vec2 texcoord) {\n"
    "   vec4 col = texture2D(tex, texcoord);\n"
    "   float s = (col[0] + col[1] + col[2]) / 3.0;\n"
    "   return vec4(s, s, s, col[3]);\n"
    "}\n";

// Shader that inverts colors.
GLchar const* const invert_src =
    "uniform sampler2D tex;\n"
    "vec4 sample_to_rgba(in vec2 texcoord) {\n"
    "   vec4 col = texture2D(tex, texcoord);\n"
    "   return vec4(1.0 - col[0], 1.0 - col[1], 1.0 - col[2], col[3]);\n"
    "}\n";

auto source_for(MirOutputFilter filter) -> GLchar const*
{
    switch (filter)
    {
    case mir_output_filter_none:
        return nullptr;
    case mir_output_filter_grayscale:
        return grayscale_src;
    case mir_output_filter_invert:
        return invert_src;
    }
    return nullptr;
}

auto compile_program(GLchar const* src) -> mrc::ProgramHandle
{
    GLchar const* vertex_src =
        "attribute vec2 position;\n"
        "attribute vec2 texcoord;\n"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "   gl_Position = vec4(position, 0, 1); \n"
        "   v_texcoord = texcoord;\n"
        "}\n";

    mrc::ShaderHandle const vertex_shader{mrc::compile_shader(GL_VERTEX_SHADER, vertex_src)};

    std::stringstream fragment_src;
    fragment_src
        <<
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        << "\n"
        << src
        << "\n"
        <<
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "    gl_FragColor = sample_to_rgba(v_texcoord);\n"
        "}\n";

    mrc::ShaderHandle const fragment_shader{
        mrc::compile_shader(GL_FRAGMENT_SHADER, fragment_src.str().c_str())};

    return mrc::link_shader(vertex_shader, fragment_shader);
}

auto make_texture() -> mrc::TextureHandle
{
    GLuint tex{0};
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    return mrc::TextureHandle{tex};
}

auto make_framebuffer(GLuint tex) -> mrc::FramebufferHandle
{
    GLuint fb{0};
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return mrc::FramebufferHandle{fb};
}
}

mrc::GLOutputFilter::GLOutputFilter()
    : texture{make_texture()},
      framebuffer{make_framebuffer(texture)}
{
}

mrc::GLOutputFilter::~GLOutputFilter() = default;

void mrc::GLOutputFilter::set_filter(MirOutputFilter filter)
{
    if (this->filter == filter)
        return;
    this->filter = filter;

    // Clear existing filter
    program = nullptr;
}

auto mrc::GLOutputFilter::active() const -> bool
{
    return source_for(filter) != nullptr;
}

auto mrc::GLOutputFilter::bind_intermediate(geom::Size size) -> bool
{
    auto const* const src = source_for(filter);
    if (!src)
        return false;

    ensure_texture_storage_for(size);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    if (program == nullptr)
    {
        program = std::make_unique<ProgramHandle>(compile_program(src));
        position_attrib = glGetAttribLocation(*program, "position");
        texcoord_attrib = glGetAttribLocation(*program, "texcoord");
        tex_uniform = glGetUniformLocation(*program, "tex");
    }

    return true;
}

void mrc::GLOutputFilter::apply()
{
    if (!active())
        return;

    glUseProgram(*program);
    glUniform1i(tex_uniform, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Draw a single right angle triangle that covers the whole output.
    GLfloat vertices[] = {-1, -1, 3, -1, -1, 3};
    GLfloat tex_coords[] = {0, 0, 2, 0, 0, 2};
    glEnableVertexAttribArray(position_attrib);
    glVertexAttribPointer(position_attrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(texcoord_attrib);
    glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 0, tex_coords);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

/// The target can be resized under us, so (re)specify the intermediate texture
/// storage whenever it no longer matches. This is only reached with a filter
/// active, so an unfiltered output never allocates texture storage.
void mrc::GLOutputFilter::ensure_texture_storage_for(geom::Size size)
{
    if (size == texture_size)
        return;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,
                 GL_RGBA,
                 size.width.as_value(),
                 size.height.as_value(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 nullptr);
    texture_size = size;
}
