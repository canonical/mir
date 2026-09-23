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

#include "gl_program_factory.h"

#include <mir/graphics/egl_error.h>

#include <boost/throw_exception.hpp>
#include <sstream>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mrc = mir::renderer::common;

namespace
{
GLchar const* const vertex_shader_src =
{
    "attribute vec3 position;\n"
    "attribute vec2 texcoord;\n"
    "uniform mat4 screen_to_gl_coords;\n"
    "uniform mat4 display_transform;\n"
    "uniform mat4 transform;\n"
    "uniform mat4 orientation_transform;\n"
    "uniform vec2 centre;\n"
    "uniform vec2 oriented_centre;\n"
    "varying vec2 v_texcoord;\n"
    "void main() {\n"
    "   vec4 mid = vec4(centre, 0.0, 0.0);\n"
    "   vec4 oriented_mid = vec4(oriented_centre, 0.0, 0.0);"
    "   vec4 transformed = (orientation_transform * (vec4(position, 1.0) - mid)) + oriented_mid;\n"
    "   transformed = (transform * (transformed - oriented_mid)) + oriented_mid;\n"
    "   gl_Position = display_transform * screen_to_gl_coords * transformed;\n"
    "   v_texcoord = texcoord;\n"
    "}\n"
};
}

auto mrc::compile_shader(GLenum type, GLchar const* src) -> GLuint
{
    GLuint id = glCreateShader(type);
    if (!id)
    {
        BOOST_THROW_EXCEPTION(mg::gl_error("Failed to create shader"));
    }

    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    GLint ok{0};
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLchar log[1024] = "(No log info)";
        glGetShaderInfoLog(id, sizeof log, nullptr, log);
        glDeleteShader(id);
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                std::string("Compile failed: ") + log + " for:\n" + src));
    }
    return id;
}

auto mrc::link_shader(
    ShaderHandle const& vertex_shader,
    ShaderHandle const& fragment_shader) -> mrc::ProgramHandle
{
    ProgramHandle program{glCreateProgram()};
    glAttachShader(program, fragment_shader);
    glAttachShader(program, vertex_shader);
    glLinkProgram(program);
    GLint ok{0};
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLchar log[1024];
        glGetProgramInfoLog(program, sizeof log - 1, nullptr, log);
        log[sizeof log - 1] = '\0';
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                std::string("Linking GL shader failed: ") + log));
    }

    return program;
}

mrc::Program::Program(ProgramHandle&& opaque_shader, ProgramHandle&& alpha_shader)
    : opaque_handle(std::move(opaque_shader)),
      alpha_handle(std::move(alpha_shader)),
      opaque{opaque_handle},
      alpha{alpha_handle}
{
}

mrc::GLProgramFactory::GLProgramFactory()
    : vertex_shader{compile_shader(GL_VERTEX_SHADER, vertex_shader_src)}
{
}

mrc::GLProgramFactory::~GLProgramFactory() = default;

auto mrc::GLProgramFactory::compile_fragment_shader(
    void const* id,
    char const* extension_fragment,
    char const* fragment_fragment) -> mg::gl::Program&
{
    /// NOTE: This does not lock the programs vector as there is one GLProgramFactory instance
    /// per rendering thread.

    for (auto const& pair : programs)
    {
        if (pair.first == id)
        {
            return *pair.second;
        }
    }

    std::stringstream opaque_fragment;
    opaque_fragment
        << extension_fragment
        << "\n"
        <<
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        << "\n"
        << fragment_fragment
        << "\n"
        <<
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "    gl_FragColor = sample_to_rgba(v_texcoord);\n"
        "}\n";

    std::stringstream alpha_fragment;
    alpha_fragment
        << extension_fragment
        << "\n"
        <<
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        << "\n"
        << fragment_fragment
        << "\n"
        <<
        "varying vec2 v_texcoord;\n"
        "uniform float alpha;\n"
        "void main() {\n"
        "    gl_FragColor = alpha * sample_to_rgba(v_texcoord);\n"
        "}\n";

    // GL shader compilation is *not* threadsafe, and requires external synchronisation
    std::lock_guard lock{compilation_mutex};

    ShaderHandle const opaque_shader{
        compile_shader(GL_FRAGMENT_SHADER, opaque_fragment.str().c_str())};
    ShaderHandle const alpha_shader{
        compile_shader(GL_FRAGMENT_SHADER, alpha_fragment.str().c_str())};

    programs.emplace_back(id, std::make_unique<Program>(
        link_shader(vertex_shader, opaque_shader),
        link_shader(vertex_shader, alpha_shader)));

    return *programs.back().second;

    // We delete opaque_shader and alpha_shader here. This is fine; it only marks them
    // for deletion. GL will only delete them once the GL Program they're linked in is destroyed.
}
