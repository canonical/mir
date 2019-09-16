/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
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
    MOCK_METHOD1(glActiveTexture, void(GLenum));
    MOCK_METHOD2(glAttachShader, void(GLuint, GLuint));
    MOCK_METHOD2(glBindBuffer, void(GLenum, GLuint));
    MOCK_METHOD2(glBindFramebuffer, void(GLenum, GLuint));
    MOCK_METHOD2(glBindRenderbuffer, void(GLenum, GLuint));
    MOCK_METHOD2(glBindTexture, void(GLenum, GLuint));
    MOCK_METHOD4(glBlendColor, void(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha));
    MOCK_METHOD2(glBlendFunc, void(GLenum, GLenum));
    MOCK_METHOD4(glBlendFuncSeparate, void(GLenum, GLenum, GLenum, GLenum));
    MOCK_METHOD4(glBufferData,
                 void(GLenum, GLsizeiptr, const GLvoid *, GLenum));
    MOCK_METHOD1(glCheckFramebufferStatus, GLenum(GLenum));
    MOCK_METHOD1(glClear, void(GLbitfield));
    MOCK_METHOD4(glClearColor, void(GLclampf, GLclampf, GLclampf, GLclampf));
    MOCK_METHOD4(glColorMask, void(GLboolean, GLboolean, GLboolean, GLboolean));
    MOCK_METHOD1(glCompileShader, void(GLuint));
    MOCK_METHOD0(glCreateProgram, GLuint());
    MOCK_METHOD1(glCreateShader, GLuint(GLenum));
    MOCK_METHOD2(glDeleteBuffers, void(GLsizei, const GLuint *));
    MOCK_METHOD2(glDeleteFramebuffers, void(GLsizei, const GLuint *));
    MOCK_METHOD2(glDeleteRenderbuffers, void(GLsizei, const GLuint *));
    MOCK_METHOD1(glDeleteProgram, void(GLuint));
    MOCK_METHOD1(glDeleteShader, void(GLuint));
    MOCK_METHOD2(glDeleteTextures, void(GLsizei, const GLuint *));
    MOCK_METHOD1(glDisable, void(GLenum));
    MOCK_METHOD1(glDisableVertexAttribArray, void(GLuint));
    MOCK_METHOD3(glDrawArrays, void(GLenum, GLint, GLsizei));
    MOCK_METHOD1(glEnable, void(GLenum));
    MOCK_METHOD1(glEnableVertexAttribArray, void(GLuint));
    MOCK_METHOD0(glFinish, void());
    MOCK_METHOD4(glFramebufferRenderbuffer,
                 void(GLenum, GLenum, GLenum, GLuint));
    MOCK_METHOD5(glFramebufferTexture2D,
                 void(GLenum, GLenum, GLenum, GLuint, GLint));
    MOCK_METHOD2(glGenBuffers, void(GLsizei, GLuint *));
    MOCK_METHOD2(glGenFramebuffers, void(GLsizei, GLuint *));
    MOCK_METHOD2(glGenRenderbuffers, void(GLsizei, GLuint*));
    MOCK_METHOD2(glGenTextures, void(GLsizei, GLuint *));
    MOCK_METHOD2(glGetAttribLocation, GLint(GLuint, const GLchar *));
    MOCK_METHOD0(glGetError, GLenum());
    MOCK_METHOD2(glGetIntegerv, void(GLenum, GLint*));
    MOCK_METHOD4(glGetProgramInfoLog,
                 void(GLuint, GLsizei, GLsizei *, GLchar *));
    MOCK_METHOD3(glGetProgramiv, void(GLuint, GLenum, GLint *));
    MOCK_METHOD4(glGetShaderInfoLog,
                 void(GLuint, GLsizei, GLsizei *, GLchar *));
    MOCK_METHOD3(glGetShaderiv, void(GLuint, GLenum, GLint *));
    MOCK_METHOD1(glGetString, const GLubyte*(GLenum));
    MOCK_METHOD2(glGetUniformLocation, GLint(GLuint, const GLchar *));
    MOCK_METHOD1(glLinkProgram, void(GLuint));
    MOCK_METHOD2(glPixelStorei, void(GLenum, GLint));
    MOCK_METHOD7(glReadPixels,
                 void(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum,
                      GLvoid*));
    MOCK_METHOD4(glRenderbufferStorage,
                 void(GLenum, GLenum, GLsizei, GLsizei));
    MOCK_METHOD4(glShaderSource,
                 void(GLuint, GLsizei, const GLchar * const *, const GLint *));
    MOCK_METHOD9(glTexImage2D,
                 void(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum,
                      GLenum,const GLvoid*));
    MOCK_METHOD3(glTexParameteri, void(GLenum, GLenum, GLenum));
    MOCK_METHOD2(glUniform1f, void(GLint, GLfloat));
    MOCK_METHOD3(glUniform2f, void(GLint, GLfloat, GLfloat));
    MOCK_METHOD2(glUniform1i, void(GLint, GLint));
    MOCK_METHOD4(glUniformMatrix4fv,
                 void(GLuint, GLsizei, GLboolean, const GLfloat *));
    MOCK_METHOD1(glUseProgram, void(GLuint));
    MOCK_METHOD6(glVertexAttribPointer,
                 void(GLuint, GLint, GLenum, GLboolean, GLsizei,
                      const GLvoid *));
    MOCK_METHOD4(glViewport, void(GLint, GLint, GLsizei, GLsizei));
    MOCK_METHOD1(glGenerateMipmap, void(GLenum target));
    MOCK_METHOD4(glDrawElements, void(GLenum, GLsizei, GLenum, const GLvoid*));
    MOCK_METHOD4(glScissor, void(GLint, GLint, GLsizei, GLsizei));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_GL_H_ */
