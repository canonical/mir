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

#include <GLES2/gl2.h>

#include <cstring>

namespace mtd = mir::test::doubles;

namespace
{
mtd::MockGL* global_mock_gl = NULL;
}

mtd::MockGL::MockGL()
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

GLenum glGetError()
{
    CHECK_GLOBAL_MOCK(GLenum);
    return global_mock_gl->glGetError();
}

const GLubyte* glGetString(GLenum name)
{
    if (!global_mock_gl)
        return nullptr;
    return global_mock_gl->glGetString(name);
}

void glUseProgram(GLuint program)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glUseProgram (program);
}

void glClear (GLbitfield mask)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glClear(mask);
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glClearColor(red, green, blue, alpha);
}

void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glColorMask(r, g, b, a);
}

void glEnable(GLenum func)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glEnable (func);
}

void glDisable(GLenum func)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glDisable(func);
}

void glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glBlendColor(red, green, blue, alpha);
}

void glBlendFunc(GLenum src, GLenum dst)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glBlendFunc (src, dst);
}

void glBlendFuncSeparate(GLenum src_rgb, GLenum dst_rgb,
                         GLenum src_alpha, GLenum dst_alpha)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glBlendFuncSeparate(src_rgb, dst_rgb, src_alpha, dst_alpha);
}

void glActiveTexture(GLenum unit)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glActiveTexture (unit);
}

void glUniformMatrix4fv(GLint location,
                        GLsizei count,
                        GLboolean transpose,
                        const GLfloat* value)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glUniformMatrix4fv(location, count, transpose, value);
}

void glUniform1f(GLint location, GLfloat x)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glUniform1f(location, x);
}

void glUniform2f(GLint location, GLfloat x, GLfloat y)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glUniform2f(location, x, y);
}

void glBindBuffer(GLenum buffer, GLuint name)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glBindBuffer(buffer, name);
}

void glVertexAttribPointer(GLuint indx,
                           GLint size,
                           GLenum type,
                           GLboolean normalized,
                           GLsizei stride,
                           const GLvoid* ptr)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
}

void glBindTexture(GLenum target, GLuint texture)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glBindTexture(target, texture);
}

void glEnableVertexAttribArray(GLuint index)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glEnableVertexAttribArray(index);
}

void glDisableVertexAttribArray(GLuint index)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glDisableVertexAttribArray(index);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glDrawArrays(mode, first, count);
}

GLuint glCreateShader(GLenum type)
{
    CHECK_GLOBAL_MOCK(GLuint);
    return global_mock_gl->glCreateShader(type);
}

void glDeleteShader(GLuint shader)
{
    CHECK_GLOBAL_VOID_MOCK();
    return global_mock_gl->glDeleteShader(shader);
}

/* This is the version of glShaderSource in Mesa < 9.0.1 */
void glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glShaderSource (shader, count, string, length);
}

/* This is the version of glShaderSource in Mesa >= 9.0.1 */
void glShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint *length)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glShaderSource (shader, count, string, length);
}

void glCompileShader(GLuint shader)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glCompileShader(shader);
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint *params)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGetShaderiv(shader, pname, params);
}

void glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *infolog)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGetShaderInfoLog(shader, bufsize, length, infolog);
}

GLuint glCreateProgram()
{
    CHECK_GLOBAL_MOCK(GLuint);
    return global_mock_gl->glCreateProgram();
}

void glDeleteProgram(GLuint program)
{
    CHECK_GLOBAL_VOID_MOCK();
    return global_mock_gl->glDeleteProgram(program);
}

void glAttachShader(GLuint program, GLuint shader)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glAttachShader(program, shader);
}

void glLinkProgram(GLuint program)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glLinkProgram(program);
}

GLint glGetUniformLocation(GLuint program, const GLchar *name)
{
    CHECK_GLOBAL_MOCK(GLint);
    return global_mock_gl->glGetUniformLocation(program, name);
}

GLint glGetAttribLocation(GLuint program, const GLchar *name)
{
    CHECK_GLOBAL_MOCK(GLint);
    return global_mock_gl->glGetAttribLocation(program, name);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glTexParameteri(target, pname, param);
}

void glGenTextures(GLsizei n, GLuint *textures)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGenTextures(n, textures);
}

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glDeleteTextures(n, textures);
}

void glUniform1i(GLint location, GLint x)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glUniform1i(location, x);
}

void glGenBuffers(GLsizei n, GLuint *buffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGenBuffers(n, buffers);
}

void glDeleteBuffers(GLsizei n, const GLuint *buffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glDeleteBuffers(n, buffers);
}

void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glBufferData(target, size, data, usage);
}

void glGetProgramiv(GLuint program, GLenum pname, GLint *params)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGetProgramiv(program, pname, params);
}

void glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei *length, GLchar *infolog)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGetProgramInfoLog(program, bufsize, length, infolog);
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type, const GLvoid* pixels)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void glGenFramebuffers(GLsizei n, GLuint *framebuffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGenFramebuffers(n, framebuffers);
}

void glDeleteFramebuffers(GLsizei n, const GLuint * framebuffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glDeleteFramebuffers(n, framebuffers);
}

void glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glBindFramebuffer(target, framebuffer);
}

void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget,
                            GLuint texture, GLint level)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glFramebufferTexture2D(target, attachment, textarget,
                                           texture, level);

}

GLenum glCheckFramebufferStatus(GLenum target)
{
    CHECK_GLOBAL_MOCK(GLenum);
    return global_mock_gl->glCheckFramebufferStatus(target);
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid* pixels)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glReadPixels(x, y, width, height, format, type, pixels);
}

void glGetIntegerv(GLenum target, GLint* params)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGetIntegerv(target, params);
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glBindRenderbuffer(target, renderbuffer);
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment,
                               GLenum renderbuffertarget, GLuint renderbuffer)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glFramebufferRenderbuffer(target, attachment,
                                              renderbuffertarget, renderbuffer);
}

void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGenRenderbuffers(n, renderbuffers);
}

void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glDeleteRenderbuffers(n, renderbuffers);
}

void glRenderbufferStorage(GLenum target, GLenum internalformat,
                           GLsizei width, GLsizei height)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glRenderbufferStorage(target, internalformat,
                                          width, height);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glViewport(x, y, width, height);
}

void glFinish()
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glFinish();
}

void glGenerateMipmap(GLenum target)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glGenerateMipmap(target);
}

void glPixelStorei(GLenum pname, GLint param)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glPixelStorei(pname, param);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indicies)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glDrawElements(mode, count, type, indicies);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    CHECK_GLOBAL_VOID_MOCK();
    global_mock_gl->glScissor(x, y, width, height);
}
