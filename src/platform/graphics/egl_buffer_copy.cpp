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

#include "./egl_debug.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "egl_buffer_copy.h"
#include "mir/graphics/egl_context_executor.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/egl_extensions.h"

#include <algorithm>
#include <cstring>
#include <exception>
#include <utility>
#include <stdexcept>

#include <boost/throw_exception.hpp>

// #define MIR_LOG_COMPONENT "egl-buffer-copy"
// #include "mir/log.h"

namespace mg = mir::graphics;

namespace
{
const GLchar* const vshader =
    {
        "attribute vec4 position;\n"
        "attribute vec2 texcoord;\n"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "   gl_Position = position;\n"
        "   v_texcoord = texcoord;\n"
        "}\n"
    };

const GLchar* const fshader =
    {
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "uniform sampler2D tex;"
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "   gl_FragColor = texture2D(tex, v_texcoord);\n"
        "}\n"
    };


template<GLuint (*allocator)(GLenum), void (* deleter)(GLuint)>
class GLTypedHandle
{
public:
    GLTypedHandle(GLenum type)
        : id{allocator(type)}
    {
    }

    ~GLTypedHandle()
    {
        if (id)
            deleter(id);
    }

    GLTypedHandle(GLTypedHandle const&) = delete;
    GLTypedHandle& operator=(GLTypedHandle const&) = delete;
    GLTypedHandle& operator=(GLTypedHandle&& rhs)
    {
        std::swap(id, rhs.id);
        return *this;
    }

    GLTypedHandle(GLTypedHandle&& from)
        : id{std::exchange(from.id, 0)}
    {
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};

using ShaderHandle = GLTypedHandle<&glCreateShader, &glDeleteShader>;

ShaderHandle compile_shader(GLenum type, GLchar const* src)
{
    auto id = ShaderHandle(type);
    if (!id)
    {
        BOOST_THROW_EXCEPTION(mg::gl_error("Failed to create shader"));
    }

    glShaderSource(id, 1, &src, NULL);
    glCompileShader(id);
    GLint ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLchar log[1024] = "(No log info)";
        glGetShaderInfoLog(id, sizeof log, NULL, log);
        glDeleteShader(id);
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                std::string("Compile failed: ") + log + " for:\n" + src));
    }
    return id;
}

ProgramHandle link_shader(
    ShaderHandle const& vertex_shader,
    ShaderHandle const& fragment_shader)
{
    ProgramHandle program;
    glAttachShader(program, fragment_shader);
    glAttachShader(program, vertex_shader);
    glLinkProgram(program);
    GLint ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLchar log[1024];
        glGetProgramInfoLog(program, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        BOOST_THROW_EXCEPTION(
            std::runtime_error(
                std::string("Linking GL shader failed: ") + log));
    }

    return program;
}

}

class mg::EGLBufferCopier::Impl
{
public:
    Impl(std::shared_ptr<mg::common::EGLContextExecutor> egl_delegate)
        : egl_delegate{std::move(egl_delegate)}
    {
        auto exception_catcher = std::make_shared<std::promise<void>>();
        auto initialised = exception_catcher->get_future();
        this->egl_delegate->spawn(
            [this, exception_catcher = std::move(exception_catcher)]()
            {
                try
                {
                    initialise();
                    exception_catcher->set_value();
                }
                catch(std::exception const& err)
                {
                    exception_catcher->set_exception(std::make_exception_ptr(err));
                }
            });
        initialised.get();
    }

    ~Impl()
    {
        // Ensure EGL resources are cleaned up in the right context
        egl_delegate->spawn([state = std::move(state)]() mutable { state = nullptr; });
    }

    auto blit(EGLImage from, EGLImage to, geometry::Size size) -> std::optional<mir::Fd>
    {
        /* TODO: It *must* be possible to create the fence FD first and *then*
         * insert it into the command stream! That would let us immediately return
         * the fd without waiting for the EGL thread.
         */
        auto sync_promise = std::make_shared<std::promise<std::optional<mir::Fd>>>();
        auto sync = sync_promise->get_future();
        egl_delegate->spawn(
            [sync = std::move(sync_promise), from, to, state = state, size]()
            {
                glUseProgram(state->prog);

                TextureHandle tex;
                RenderbufferHandle renderbuffer;
                FramebufferHandle fbo;

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tex);
                state->glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, from);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
                state->glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, to);


                glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);

                glBindBuffer(GL_ARRAY_BUFFER, state->vert_data);
                glVertexAttribPointer (state->attrpos, 2, GL_FLOAT, GL_FALSE, 0, 0);

                glBindBuffer(GL_ARRAY_BUFFER, state->tex_data);
                glVertexAttribPointer (state->attrtex, 2, GL_FLOAT, GL_FALSE, 0, 0);

                glEnableVertexAttribArray(state->attrpos);
                glEnableVertexAttribArray(state->attrtex);

                glViewport(0, 0, size.width.as_int(), size.height.as_int());
                GLubyte const idx[] = { 0, 1, 3, 2 };
                glDrawElements (GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_BYTE, idx);

                static int blit_count = 0;

                if(blit_count % 60 == 0)
                {
                    auto _glEGLImageTargetTexture2DOES = [&state](auto target, auto egl_image)
                    {
                        state->glEGLImageTargetTexture2DOES(target, egl_image);
                    };
                    eglimage_to_ppm(_glEGLImageTargetTexture2DOES, from, size.width.as_int(), size.height.as_int(), GL_RGBA, std::format("from-{}.ppm", blit_count));
                    eglimage_to_ppm(_glEGLImageTargetTexture2DOES, to, size.width.as_int(), size.height.as_int(), GL_RGBA, std::format("to-{}.ppm", blit_count));
                }

                blit_count++;

                // TODO: Actually use the sync
                glFinish();
                sync->set_value({});

                // Unbind all our resources
                glBindTexture(GL_TEXTURE_2D, 0);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
            });
        return sync.get();
    }
private:
    std::shared_ptr<mg::common::EGLContextExecutor> const egl_delegate;
    // State accessed only on the EGL thread
    std::shared_ptr<mg::EGLExtensions> const egl_extensions;
    struct State
    {
        ProgramHandle prog;
        GLint attrtex;
        GLint attrpos;
        GLBufferHandle vert_data;
        GLBufferHandle tex_data;
        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES;
        PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glEGLImageTargetRenderbufferStorageOES;
    };
    /* This has to be initialised on the EGL thread, but it contains non-default-initialisable state.
     *
     * We use a shared_ptr to avoid worrying about making sure we outlive any invocations on the EGL thread.
     * We don't need any synchronisation because state is only ever accessed on EGLContextExecutor, which
     * is single-threaded.
     */
    std::shared_ptr<State> state;

    // Called once, from the EGL thread
    void initialise()
    {
        state = std::make_shared<State>();

        // Check necessary EGL extensions
        if (!strstr(reinterpret_cast<char const*>(glGetString(GL_EXTENSIONS)), "GL_OES_EGL_image"))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"Missing required GL_OES_EGL_image extension"}));
        }
        state->glEGLImageTargetRenderbufferStorageOES =
            reinterpret_cast<PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC>(eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES"));
        state->glEGLImageTargetTexture2DOES =
            reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

        state->prog = link_shader(compile_shader(GL_VERTEX_SHADER, vshader), compile_shader(GL_FRAGMENT_SHADER, fshader));

        glUseProgram(state->prog);

        state->attrpos = glGetAttribLocation(state->prog, "position");
        state->attrtex = glGetAttribLocation(state->prog, "texcoord");

        auto unitex = glGetUniformLocation(state->prog, "tex");

        glUniform1i(unitex, 0);

        static GLfloat const dest_vert[4][2] =
            { { -1.f, 1.f }, { 1.f, 1.f }, { 1.f, -1.f }, { -1.f, -1.f } };
        glBindBuffer(GL_ARRAY_BUFFER, state->vert_data);
        glBufferData(GL_ARRAY_BUFFER, sizeof(dest_vert), dest_vert, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        static GLfloat const tex_vert[4][2] =
            {
                { 0.f, 0.f }, { 1.f, 0.f }, { 1.f, 1.f }, { 0.f, 1.f },
            };
        glBindBuffer(GL_ARRAY_BUFFER, state->tex_data);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tex_vert), tex_vert, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

mg::EGLBufferCopier::EGLBufferCopier(std::shared_ptr<mg::common::EGLContextExecutor> egl_delegate)
    : impl{std::make_unique<Impl>(std::move(egl_delegate))}
{
}

mg::EGLBufferCopier::~EGLBufferCopier() = default;

auto mg::EGLBufferCopier::blit(EGLImage src, EGLImage dest, geometry::Size size) -> std::optional<mir::Fd>
{
    return impl->blit(src, dest, size);
}
