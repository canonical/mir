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

#ifndef MIR_RENDERER_COMMON_GL_HANDLES_H_
#define MIR_RENDERER_COMMON_GL_HANDLES_H_

#include <GLES2/gl2.h>
#include <utility>

namespace mir::renderer::common
{

/// Owning handle for GL objects deleted one-at-a-time (shaders, programs)
template<void (* deleter)(GLuint)>
class GLHandle
{
public:
    GLHandle() = default;

    explicit GLHandle(GLuint id)
        : id{id}
    {
    }

    ~GLHandle()
    {
        if (id)
            (*deleter)(id);
    }

    GLHandle(GLHandle const&) = delete;

    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from)
        : id{from.id}
    {
        from.id = 0;
    }

    GLHandle& operator=(GLHandle&& from)
    {
        std::swap(id, from.id);
        return *this;
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id{0};
};

using ProgramHandle = GLHandle<&glDeleteProgram>;
using ShaderHandle = GLHandle<&glDeleteShader>;

/// Owning handle for GL objects deleted in batches (textures, framebuffers, renderbuffers)
template<void (* deleter)(GLsizei, const GLuint*)>
class GLMultiHandle
{
public:
    GLMultiHandle() = default;

    explicit GLMultiHandle(GLuint id)
        : id{id}
    {
    }

    ~GLMultiHandle()
    {
        if (id)
            (*deleter)(1, &id);
    }

    GLMultiHandle(GLMultiHandle const&) = delete;

    GLMultiHandle& operator=(GLMultiHandle const&) = delete;

    GLMultiHandle(GLMultiHandle&& from)
        : id{from.id}
    {
        from.id = 0;
    }

    GLMultiHandle& operator=(GLMultiHandle&& from)
    {
        std::swap(id, from.id);
        return *this;
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id{0};
};

using TextureHandle = GLMultiHandle<&glDeleteTextures>;
using FramebufferHandle = GLMultiHandle<&glDeleteFramebuffers>;

}

#endif // MIR_RENDERER_COMMON_GL_HANDLES_H_
