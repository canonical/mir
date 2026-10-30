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

#ifndef MIR_GRAPHICS_GL_HANDLES_H_
#define MIR_GRAPHICS_GL_HANDLES_H_

#include <GLES2/gl2.h>
#include <concepts>
#include <utility>

namespace mir::graphics
{
/**
 * RAII owner of a GL object name.
 *
 * \tparam deleter   void(GLuint)                 (eg: glDeleteProgram), or
 *                   void(GLsizei, GLuint const*) (eg: glDeleteTextures)
 * \tparam allocator GLuint()                     (eg: glCreateProgram),
 *                   GLuint(GLenum)               (eg: glCreateShader), or
 *                   void(GLsizei, GLuint*)       (eg: glGenTextures)
 */
template<auto deleter, auto allocator>
class GLHandle
{
public:
    /// eg: glCreateProgram()
    GLHandle()
        requires std::invocable<decltype(allocator)>
        : id{allocator()}
    {}

    /// eg: glGenTextures(1, &id)
    GLHandle()
        requires std::invocable<decltype(allocator), GLsizei, GLuint*>
        : id{0}
    {
        allocator(1, &id);
    }

    /// eg: glCreateShader(GL_VERTEX_SHADER)
    explicit GLHandle(GLenum type)
        requires std::invocable<decltype(allocator), GLenum>
        : id{allocator(type)}
    {}

    ~GLHandle() { reset(); }

    void reset()
    {
        if (id)
        {
            if constexpr (std::invocable<decltype(deleter), GLsizei, GLuint const*>)
                deleter(1, &id);
            else
                deleter(id);
            id = 0;
        }
    }

    GLHandle(GLHandle const&) = delete;
    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from) noexcept : id{std::exchange(from.id, 0)} {}
    GLHandle& operator=(GLHandle&& rhs) noexcept
    {
        std::swap(id, rhs.id);
        return *this;
    }

    operator GLuint() const noexcept { return id; }

private:
    GLuint id{0};
};

using ProgramHandle = GLHandle<&glDeleteProgram, &glCreateProgram>;
using ShaderHandle = GLHandle<&glDeleteShader, &glCreateShader>;
using TextureHandle = GLHandle<&glDeleteTextures, &glGenTextures>;
using BufferHandle = GLHandle<&glDeleteBuffers, &glGenBuffers>;
using GLBufferHandle = BufferHandle;
using RenderbufferHandle = GLHandle<&glDeleteRenderbuffers, &glGenRenderbuffers>;
using FramebufferHandle = GLHandle<&glDeleteFramebuffers, &glGenFramebuffers>;
}

#endif // MIR_GRAPHICS_GL_HANDLES_H_
