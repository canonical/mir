/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/test/doubles/mock_gl.h"
#include <gtest/gtest.h>

#include <cstring>

namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
mtd::MockGL* global_mock_gl = NULL;

auto filter_gles(decltype(&dlopen) real_dlopen, char const* filename, int flags) -> std::optional<void*>
{
    if (strcmp(filename, "libGLESv2.so.2") == 0)
    {
        return {real_dlopen(nullptr, flags)};
    }
    return std::optional<void*>{};
}
}

mtd::MockGL::MockGL()
    : libgl_interposer{
    mtf::add_dlopen_filter(&filter_gles)}
{
    using namespace testing;
    assert(global_mock_gl == NULL && "Only one mock object per process is allowed");

    global_mock_gl = this;

    ON_CALL(*this, glCheckFramebufferStatus(_))
        .WillByDefault(Return(GL_FRAMEBUFFER_COMPLETE));
    ON_CALL(*this, glGetShaderiv(_,_,_))
        .WillByDefault(SetArgPointee<2>(GL_TRUE));
    ON_CALL(*this, glGetProgramiv(_,_,_))
        .WillByDefault(SetArgPointee<2>(GL_TRUE));
    ON_CALL(*this, glGenTextures(_, _))
        .WillByDefault(Invoke(
            [] (GLsizei n, GLuint *textures)
            {
                std::memset(textures, 0, n * sizeof(*textures));
            }));
}

void mtd::MockGL::provide_gles_extensions()
{
    using namespace testing;
    const char* gl_exts = "GL_OES_EGL_image";

    ON_CALL(*this, glGetString(GL_EXTENSIONS))
        .WillByDefault(Return(reinterpret_cast<const GLubyte*>(gl_exts)));
}

mtd::MockGL::~MockGL()
{
    global_mock_gl = NULL;
}

#define CHECK_GLOBAL_VOID_MOCK()            \
    if (!global_mock_gl)                    \
    {                                       \
        using namespace ::testing;          \
        ADD_FAILURE_AT(__FILE__,__LINE__);  \
        return;                             \
    }

#define CHECK_GLOBAL_MOCK(rettype)          \
    if (!global_mock_gl)                    \
    {                                       \
        using namespace ::testing;          \
        ADD_FAILURE_AT(__FILE__,__LINE__);  \
        rettype type = (rettype) 0;         \
        return type;                        \
    }

#undef glGetError
extern "C"
{
GLenum glGetError();
};
GLenum glGetError()
{
    CHECK_GLOBAL_MOCK(GLenum);
    return global_mock_gl->epoxy_glGetError();
}

#undef glGetString
extern "C"
{
const GLubyte* glGetString(GLenum name);
}
const GLubyte* glGetString(GLenum name)
{
    if (!global_mock_gl)
        return nullptr;
    return global_mock_gl->epoxy_glGetString(name);
}

#undef glUseProgram
extern "C"
{
void glUseProgram(GLuint program);
}
void glUseProgram(GLuint program)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glUseProgram (program);
}

#undef glClear
extern "C"
{
void glClear (GLbitfield mask);
}
void glClear (GLbitfield mask)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glClear(mask);
}

#undef glClearColor
extern "C"
{
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
}
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glClearColor(red, green, blue, alpha);
}

#undef glColorMask
extern "C"
{
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a);
}
void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glColorMask(r, g, b, a);
}

#undef glEnable
extern "C"
{
void glEnable(GLenum func);
}
void glEnable(GLenum func)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glEnable (func);
}

#undef glDisable
extern "C"
{
void glDisable(GLenum func);
}
void glDisable(GLenum func)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glDisable(func);
}

#undef glBlendColor
extern "C"
{
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
}
void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glBlendColor(red, green, blue, alpha);
}

#undef glBlendFunc
extern "C"
{
void glBlendFunc(GLenum src, GLenum dst);
}
void glBlendFunc(GLenum src, GLenum dst)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glBlendFunc (src, dst);
}

#undef glBlendFuncSeparate
extern "C"
{
void glBlendFuncSeparate(GLenum src_rgb, GLenum dst_rgb,
                         GLenum src_alpha, GLenum dst_alpha);
}
void glBlendFuncSeparate(GLenum src_rgb, GLenum dst_rgb,
                         GLenum src_alpha, GLenum dst_alpha)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glBlendFuncSeparate(src_rgb, dst_rgb, src_alpha, dst_alpha);
}

#undef glActiveTexture
extern "C"
{
void glActiveTexture(GLenum unit);
}
void glActiveTexture(GLenum unit)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glActiveTexture (unit);
}

#undef glUniformMatrix4fv
extern "C"
{
void glUniformMatrix4fv(GLint location,
                        GLsizei count,
                        GLboolean transpose,
                        const GLfloat* value);
}
void glUniformMatrix4fv(GLint location,
                        GLsizei count,
                        GLboolean transpose,
                        const GLfloat* value)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glUniformMatrix4fv(location, count, transpose, value);
}

#undef glUniform1f
extern "C"
{
void glUniform1f(GLint location, GLfloat x);
}
void glUniform1f(GLint location, GLfloat x)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glUniform1f(location, x);
}

#undef glUniform2f
extern "C"
{
void glUniform2f(GLint location, GLfloat x, GLfloat y);
}
void glUniform2f(GLint location, GLfloat x, GLfloat y)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glUniform2f(location, x, y);
}

#undef glBindBuffer
extern "C"
{
void glBindBuffer(GLenum buffer, GLuint name);
}
void glBindBuffer(GLenum buffer, GLuint name)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glBindBuffer(buffer, name);
}

#undef glVertexAttribPointer
extern "C"
{
void glVertexAttribPointer(GLuint indx,
                           GLint size,
                           GLenum type,
                           GLboolean normalized,
                           GLsizei stride,
                           const GLvoid* ptr);
}
void glVertexAttribPointer(GLuint indx,
                           GLint size,
                           GLenum type,
                           GLboolean normalized,
                           GLsizei stride,
                           const GLvoid* ptr)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
}

#undef glBindTexture
extern "C"
{
void glBindTexture(GLenum target, GLuint texture);
}
void glBindTexture(GLenum target, GLuint texture)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glBindTexture(target, texture);
}

#undef glEnableVertexAttribArray
extern "C"
{
void glEnableVertexAttribArray(GLuint index);
}
void glEnableVertexAttribArray(GLuint index)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glEnableVertexAttribArray(index);
}

#undef glDisableVertexAttribArray
extern "C"
{
void glDisableVertexAttribArray(GLuint index);
}
void glDisableVertexAttribArray(GLuint index)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glDisableVertexAttribArray(index);
}

#undef glDrawArrays
extern "C"
{
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
}
void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glDrawArrays(mode, first, count);
}

#undef glCreateShader
extern "C"
{
GLuint glCreateShader(GLenum type);
}
GLuint glCreateShader(GLenum type)
{
    CHECK_GLOBAL_MOCK(GLuint);
    return global_mock_gl->epoxy_glCreateShader(type);
}

#undef glDeleteShader
extern "C"
{
void glDeleteShader(GLuint shader);
}
void glDeleteShader(GLuint shader)
{
    CHECK_GLOBAL_VOID_MOCK();
    return global_mock_gl->epoxy_glDeleteShader(shader);
}

#undef glShaderSource
extern "C"
{
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint *length);
}
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint *length)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glShaderSource (shader, count, string, length);
}

#undef glCompileShader
extern "C"
{
void glCompileShader(GLuint shader);
}
void glCompileShader(GLuint shader)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glCompileShader(shader);
}

#undef glGetShaderiv
extern "C"
{
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params);
}
void glGetShaderiv(GLuint shader, GLenum pname, GLint *params)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGetShaderiv(shader, pname, params);
}

#undef glGetShaderInfoLog
extern "C"
{
void glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *infolog);
}
void glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *infolog)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGetShaderInfoLog(shader, bufsize, length, infolog);
}

#undef glCreateProgram
extern "C"
{
GLuint glCreateProgram();
}
GLuint glCreateProgram()
{
    CHECK_GLOBAL_MOCK(GLuint);
    return global_mock_gl->epoxy_glCreateProgram();
}

#undef glDeleteProgram
extern "C"
{
void glDeleteProgram(GLuint program);
}
void glDeleteProgram(GLuint program)
{
    CHECK_GLOBAL_VOID_MOCK();
    return global_mock_gl->epoxy_glDeleteProgram(program);
}

#undef glAttachShader
extern "C"
{
void glAttachShader(GLuint program, GLuint shader);
}
void glAttachShader(GLuint program, GLuint shader)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glAttachShader(program, shader);
}

#undef glLinkProgram
extern "C"
{
void glLinkProgram(GLuint program);
}
void glLinkProgram(GLuint program)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glLinkProgram(program);
}

#undef glGetUniformLocation
extern "C"
{
GLint glGetUniformLocation(GLuint program, const GLchar *name);
}
GLint glGetUniformLocation(GLuint program, const GLchar *name)
{
    CHECK_GLOBAL_MOCK(GLint);
    return global_mock_gl->epoxy_glGetUniformLocation(program, name);
}

#undef glGetAttribLocation
extern "C"
{
GLint glGetAttribLocation(GLuint program, const GLchar *name);
}
GLint glGetAttribLocation(GLuint program, const GLchar *name)
{
    CHECK_GLOBAL_MOCK(GLint);
    return global_mock_gl->epoxy_glGetAttribLocation(program, name);
}

#undef glTexParameteri
extern "C"
{
void glTexParameteri(GLenum target, GLenum pname, GLint param);
}
void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glTexParameteri(target, pname, param);
}

#undef glGenTextures
extern "C"
{
void glGenTextures(GLsizei n, GLuint *textures);
}
void glGenTextures(GLsizei n, GLuint *textures)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGenTextures(n, textures);
}

#undef glDeleteTextures
extern "C"
{
void glDeleteTextures(GLsizei n, const GLuint *textures);
}
void glDeleteTextures(GLsizei n, const GLuint *textures)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glDeleteTextures(n, textures);
}

#undef glUniform1i
extern "C"
{
void glUniform1i(GLint location, GLint x);
}
void glUniform1i(GLint location, GLint x)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glUniform1i(location, x);
}

#undef glGenBuffers
extern "C"
{
void glGenBuffers(GLsizei n, GLuint *buffers);
}
void glGenBuffers(GLsizei n, GLuint *buffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGenBuffers(n, buffers);
}

#undef glDeleteBuffers
extern "C"
{
void glDeleteBuffers(GLsizei n, const GLuint *buffers);
}
void glDeleteBuffers(GLsizei n, const GLuint *buffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glDeleteBuffers(n, buffers);
}

#undef glBufferData
extern "C"
{
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
}
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glBufferData(target, size, data, usage);
}

#undef glGetProgramiv
extern "C"
{
void glGetProgramiv(GLuint program, GLenum pname, GLint *params);
}
void glGetProgramiv(GLuint program, GLenum pname, GLint *params)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGetProgramiv(program, pname, params);
}

#undef glGetProgramInfoLog
extern "C"
{
void glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei *length, GLchar *infolog);
}
void glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei *length, GLchar *infolog)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGetProgramInfoLog(program, bufsize, length, infolog);
}

#undef glTexImage2D
extern "C"
{
void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid* pixels);
}
void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid* pixels)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

#undef glGenFramebuffers
extern "C"
{
void glGenFramebuffers(GLsizei n, GLuint *framebuffers);
}
void glGenFramebuffers(GLsizei n, GLuint *framebuffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGenFramebuffers(n, framebuffers);
}

#undef glDeleteFramebuffers
extern "C"
{
void glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers);
}
void glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glDeleteFramebuffers(n, framebuffers);
}

#undef glBindFramebuffer
extern "C"
{
void glBindFramebuffer(GLenum target, GLuint framebuffer);
}
void glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glBindFramebuffer(target, framebuffer);
}

#undef glFramebufferTexture2D
extern "C"
{
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                            GLuint texture, GLint level);
}
void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                            GLuint texture, GLint level)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glFramebufferTexture2D(target, attachment, textarget,
                                           texture, level);

}

#undef glCheckFramebufferStatus
extern "C"
{
GLenum glCheckFramebufferStatus(GLenum target);
}
GLenum glCheckFramebufferStatus(GLenum target)
{
    CHECK_GLOBAL_MOCK(GLenum);
    return global_mock_gl->epoxy_glCheckFramebufferStatus(target);
}

#undef glReadPixels
extern "C"
{
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid* pixels);
}
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid* pixels)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glReadPixels(x, y, width, height, format, type, pixels);
}

#undef glGetIntegerv
extern "C"
{
void glGetIntegerv(GLenum target, GLint* params);
}
void glGetIntegerv(GLenum target, GLint* params)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGetIntegerv(target, params);
}

#undef glBindRenderbuffer
extern "C"
{
void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
}
void glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glBindRenderbuffer(target, renderbuffer);
}

#undef glFramebufferRenderbuffer
extern "C"
{
void glFramebufferRenderbuffer(GLenum target, GLenum attachment,
                               GLenum renderbuffertarget, GLuint renderbuffer);
}
void glFramebufferRenderbuffer(GLenum target, GLenum attachment,
                               GLenum renderbuffertarget, GLuint renderbuffer)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glFramebufferRenderbuffer(target, attachment,
                                              renderbuffertarget, renderbuffer);
}

#undef glGenRenderbuffers
extern "C"
{
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers);
}
void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGenRenderbuffers(n, renderbuffers);
}

#undef glDeleteRenderbuffers
extern "C"
{
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers);
}
void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glDeleteRenderbuffers(n, renderbuffers);
}

#undef glRenderbufferStorage
extern "C"
{
void glRenderbufferStorage(GLenum target, GLenum internalformat,
                           GLsizei width, GLsizei height);
}
void glRenderbufferStorage(GLenum target, GLenum internalformat,
                           GLsizei width, GLsizei height)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glRenderbufferStorage(target, internalformat,
                                          width, height);
}

#undef glViewport
extern "C"
{
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
}
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glViewport(x, y, width, height);
}

#undef glFinish
extern "C"
{
void glFinish();
}
void glFinish()
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glFinish();
}

#undef glGenerateMipmap
extern "C"
{
void glGenerateMipmap(GLenum target);
}
void glGenerateMipmap(GLenum target)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glGenerateMipmap(target);
}

#undef glPixelStorei
extern "C"
{
void glPixelStorei(GLenum pname, GLint param);
}
void glPixelStorei(GLenum pname, GLint param)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glPixelStorei(pname, param);
}

#undef glDrawElements
extern "C"
{
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indicies);
}
void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indicies)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glDrawElements(mode, count, type, indicies);
}

#undef glScissor
extern "C"
{
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
}
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->epoxy_glScissor(x, y, width, height);
}
