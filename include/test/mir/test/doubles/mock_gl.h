/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_TEST_DOUBLES_MOCK_GL_H_
#define MIR_TEST_DOUBLES_MOCK_GL_H_

#include <gmock/gmock.h>
#include <GLES2/gl2.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockGL
{
public:
    MockGL();
    ~MockGL();
    void provide_gles_extensions();

    // Please keep these ordered by name
    MOCK_METHOD(void, glActiveTexture, (GLenum));
    MOCK_METHOD(void, glAttachShader, (GLuint, GLuint));
    MOCK_METHOD(void, glBindBuffer, (GLenum, GLuint));
    MOCK_METHOD(void, glBindFramebuffer, (GLenum, GLuint));
    MOCK_METHOD(void, glBindRenderbuffer, (GLenum, GLuint));
    MOCK_METHOD(void, glBindTexture, (GLenum, GLuint));
    MOCK_METHOD(void, glBlendColor, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha));
    MOCK_METHOD(void, glBlendFunc, (GLenum, GLenum));
    MOCK_METHOD(void, glBlendFuncSeparate, (GLenum, GLenum, GLenum, GLenum));
    MOCK_METHOD(void, glBufferData,
                 (GLenum, GLsizeiptr, const GLvoid *, GLenum));
    MOCK_METHOD(GLenum, glCheckFramebufferStatus, (GLenum));
    MOCK_METHOD(void, glClear, (GLbitfield));
    MOCK_METHOD(void, glClearColor, (GLclampf, GLclampf, GLclampf, GLclampf));
    MOCK_METHOD(void, glColorMask, (GLboolean, GLboolean, GLboolean, GLboolean));
    MOCK_METHOD(void, glCompileShader, (GLuint));
    MOCK_METHOD(GLuint, glCreateProgram, ());
    MOCK_METHOD(GLuint, glCreateShader, (GLenum));
    MOCK_METHOD(void, glDeleteBuffers, (GLsizei, const GLuint *));
    MOCK_METHOD(void, glDeleteFramebuffers, (GLsizei, const GLuint *));
    MOCK_METHOD(void, glDeleteRenderbuffers, (GLsizei, const GLuint *));
    MOCK_METHOD(void, glDeleteProgram, (GLuint));
    MOCK_METHOD(void, glDeleteShader, (GLuint));
    MOCK_METHOD(void, glDeleteTextures, (GLsizei, const GLuint *));
    MOCK_METHOD(void, glDisable, (GLenum));
    MOCK_METHOD(void, glDisableVertexAttribArray, (GLuint));
    MOCK_METHOD(void, glDrawArrays, (GLenum, GLint, GLsizei));
    MOCK_METHOD(void, glEnable, (GLenum));
    MOCK_METHOD(void, glEnableVertexAttribArray, (GLuint));
    MOCK_METHOD(void, glFinish, ());
    MOCK_METHOD(void, glFlush, ());
    MOCK_METHOD(void, glFramebufferRenderbuffer,
                 (GLenum, GLenum, GLenum, GLuint));
    MOCK_METHOD(void, glFramebufferTexture2D,
                 (GLenum, GLenum, GLenum, GLuint, GLint));
    MOCK_METHOD(void, glGenBuffers, (GLsizei, GLuint *));
    MOCK_METHOD(void, glGenFramebuffers, (GLsizei, GLuint *));
    MOCK_METHOD(void, glGenRenderbuffers, (GLsizei, GLuint*));
    MOCK_METHOD(void, glGenTextures, (GLsizei, GLuint *));
    MOCK_METHOD(GLint, glGetAttribLocation, (GLuint, const GLchar *));
    MOCK_METHOD(GLenum, glGetError, ());
    MOCK_METHOD(void, glGetIntegerv, (GLenum, GLint*));
    MOCK_METHOD(void, glGetProgramInfoLog,
                 (GLuint, GLsizei, GLsizei *, GLchar *));
    MOCK_METHOD(void, glGetProgramiv, (GLuint, GLenum, GLint *));
    MOCK_METHOD(void, glGetShaderInfoLog,
                 (GLuint, GLsizei, GLsizei *, GLchar *));
    MOCK_METHOD(void, glGetShaderiv, (GLuint, GLenum, GLint *));
    MOCK_METHOD(const GLubyte*, glGetString, (GLenum));
    MOCK_METHOD(GLint, glGetUniformLocation, (GLuint, const GLchar *));
    MOCK_METHOD(void, glLinkProgram, (GLuint));
    MOCK_METHOD(void, glPixelStorei, (GLenum, GLint));
    MOCK_METHOD(void, glReadPixels,
                 (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid*));
    MOCK_METHOD(void, glRenderbufferStorage,
                (GLenum, GLenum, GLsizei, GLsizei));
    MOCK_METHOD(void, glShaderSource,
                (GLuint, GLsizei, const GLchar * const *, const GLint *));
    MOCK_METHOD(void, glTexImage2D,
                (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum,const GLvoid*));
    MOCK_METHOD(void, glTexParameteri, (GLenum, GLenum, GLenum));
    MOCK_METHOD(void, glUniform1f, (GLint, GLfloat));
    MOCK_METHOD(void, glUniform2f, (GLint, GLfloat, GLfloat));
    MOCK_METHOD(void, glUniform1i, (GLint, GLint));
    MOCK_METHOD(void, glUniformMatrix4fv,
                 (GLuint, GLsizei, GLboolean, const GLfloat *));
    MOCK_METHOD(void, glUseProgram, (GLuint));
    MOCK_METHOD(void, glVertexAttribPointer,
                 (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *));
    MOCK_METHOD(void, glViewport, (GLint, GLint, GLsizei, GLsizei));
    MOCK_METHOD(void, glGenerateMipmap, (GLenum target));
    MOCK_METHOD(void, glDrawElements, (GLenum, GLsizei, GLenum, const GLvoid*));
    MOCK_METHOD(void, glScissor, (GLint, GLint, GLsizei, GLsizei));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_GL_H_ */
