/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_GL_PROGRAM_H_
#define MIR_GRAPHICS_GL_PROGRAM_H_

#include <GLES2/gl2.h>

namespace mir
{
namespace graphics
{

class GLShader
{
public:
    GLShader(GLchar const* shader_src, GLuint type);
    ~GLShader();
    operator GLuint() const;

private:
    GLShader(GLShader const&) = delete;
    GLShader& operator=(GLShader const&) = delete;

    GLuint const shader;
};

class GLProgram
{
public:
    virtual ~GLProgram() = default;
    virtual operator GLuint() const = 0;

protected:
    GLProgram() = default;
private:
    GLProgram(GLProgram const&) = delete;
    GLProgram& operator=(GLProgram const&) = delete;
};

class SimpleGLProgram : public GLProgram
{
public:
    SimpleGLProgram(
        GLchar const* vertex_shader_src,
        GLchar const* fragment_shader_src);
    ~SimpleGLProgram();

    operator GLuint() const override;
private:
    GLShader const vertex_shader; 
    GLShader const fragment_shader;
    GLuint const program;
};

}
}

#endif /* MIR_GRAPHICS_GL_PROGRAM_H_ */
