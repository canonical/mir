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

#ifndef MIR_GL_PROGRAM_H_
#define MIR_GL_PROGRAM_H_

#include <GLES2/gl2.h>

namespace mir
{
namespace gl
{

class Shader
{
public:
    Shader(GLchar const* shader_src, GLuint type);
    ~Shader();
    operator GLuint() const;

private:
    Shader(Shader const&) = delete;
    Shader& operator=(Shader const&) = delete;

    GLuint const shader;
};

class Program
{
public:
    virtual ~Program() = default;
    virtual operator GLuint() const = 0;

protected:
    Program() = default;
private:
    Program(Program const&) = delete;
    Program& operator=(Program const&) = delete;
};

class SimpleProgram : public Program
{
public:
    SimpleProgram(
        GLchar const* vertex_shader_src,
        GLchar const* fragment_shader_src);
    ~SimpleProgram();

    operator GLuint() const override;
private:
    Shader const vertex_shader;
    Shader const fragment_shader;
    GLuint const program;
};

}
}

#endif /* MIR_GL_PROGRAM_H_ */
