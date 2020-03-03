/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_PROGRAM_FACTORY_H_
#define MIR_PLATFORM_PROGRAM_FACTORY_H_

#include <memory>

namespace mir
{
namespace graphics
{
namespace gl
{
class Program;

class ProgramFactory
{
public:
    ProgramFactory() = default;
    virtual ~ProgramFactory();

    ProgramFactory(ProgramFactory const&) = delete;
    ProgramFactory& operator=(ProgramFactory const&) = delete;

    /**
     * Compile and link a fragment-shader fragment into a full shader Program
     *
     * \note    As the id needs to be globally unique, we suggest using the address of a local
     *          static variable as the id.
     *
     * \param id [in]                   An opaque ID for this shader. This *must* be globally unique
     * \param extension_fragment [in]   Fragment to include necessary extensions
     * \param fragment_fragment [in]    The fragment-shader fragment. This must be a GLSL
     *                                  fragment defining a function:
     *                                  vec4 sample_to_rgba(in vec2 texcoord)
     *                                  returning a vector containing the RGBA value at texcoord.
     *                                  The elements of the uniform array tex[n] will be bound to
     *                                  GL_TEXTURE0, GL_TEXTURE1, …, GL_TEXTURE(n-1). (n <= 8)
     * \return  A reference to the fully compiled and linked Program
     */
    virtual Program& compile_fragment_shader(
        void* id,
        char const* extension_fragment,
        char const* fragment_fragment) = 0;
};
}
}
}

#endif //MIR_PLATFORM_PROGRAM_FACTORY_H_
