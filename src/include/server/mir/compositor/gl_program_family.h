/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_COMPOSITOR_GL_PROGRAM_FAMILY_H_
#define MIR_COMPOSITOR_GL_PROGRAM_FAMILY_H_

#include <GLES2/gl2.h>
#include <utility>
#include <map>

namespace mir { namespace compositor {

class GLProgramFamily
{
public:
    GLuint add_program(const char* vshader, const char* fshader);

    /// A faster cached version of glGetUniformLocation()
    GLint get_uniform_location(GLuint program_id, const char* name) const;

private:
    typedef std::pair<GLenum, const char*> ShaderKey;
    typedef GLuint ShaderId;
    struct Shader
    {
        ShaderId id = 0;
        void init(GLenum type, const char* src);
        ~Shader();
    };
    typedef std::map<const char*, Shader> ShaderMap;
    ShaderMap vshader, fshader;

    typedef GLuint ProgramId;
    typedef std::pair<ShaderId, ShaderId> ShaderPair;
    struct Program
    {
        ProgramId id = 0;
        ~Program();
    };
    std::map<ShaderPair, Program> program;

    typedef std::pair<ProgramId, const char*> UniformKey;
    typedef GLint UniformId;
    struct Uniform
    {
        UniformId id = -1;
    };
    typedef std::map<UniformKey, Uniform> UniformMap;
    mutable UniformMap uniform;
};

}}  // namespace mir::compositor

#endif // MIR_COMPOSITOR_GL_PROGRAM_FAMILY_H_
