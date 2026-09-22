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

#ifndef MIR_RENDERER_COMMON_GL_PROGRAM_FACTORY_H_
#define MIR_RENDERER_COMMON_GL_PROGRAM_FACTORY_H_

#include "gl_handles.h"

#include <mir/graphics/program.h>
#include <mir/graphics/program_factory.h>
#include <mir/renderers/gl/renderer.h>

#include <memory>
#include <mutex>
#include <vector>

namespace mir::renderer::common
{

/**
 * The opaque and alpha-blended variants of the program for one texture type.
 *
 * `graphics::gl::Texture::shader()` returns one of these (as its base class);
 * drawing code selects the variant appropriate for the renderable's alpha.
 */
struct ProgramFamily : public graphics::gl::Program
{
    ProgramFamily(ProgramHandle&& opaque_shader, ProgramHandle&& alpha_shader);

    ProgramHandle const opaque_handle, alpha_handle;
    gl::Renderer::Program opaque, alpha;
};

/**
 * Compiles and caches the scene-drawing shader programs.
 *
 * \note  Instances must be constructed, used, and destroyed with the GL context
 *        the programs belong to current.
 */
class GLProgramFactory : public graphics::gl::ProgramFactory
{
public:
    GLProgramFactory();
    ~GLProgramFactory();

    auto compile_fragment_shader(
        void const* id,
        char const* extension_fragment,
        char const* fragment_fragment) -> graphics::gl::Program& override;

private:
    ShaderHandle const vertex_shader;
    std::vector<std::pair<void const*, std::unique_ptr<ProgramFamily>>> programs;
    // GL requires us to synchronise multi-threaded access to the shader APIs.
    std::mutex compilation_mutex;
};

/// Compile a shader of `type` from `src`, throwing on failure
auto compile_shader(GLenum type, GLchar const* src) -> GLuint;

/// Link `vertex_shader` and `fragment_shader` into a program, throwing on failure
auto link_shader(ShaderHandle const& vertex_shader, ShaderHandle const& fragment_shader) -> ProgramHandle;

}

#endif // MIR_RENDERER_COMMON_GL_PROGRAM_FACTORY_H_
