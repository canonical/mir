/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_COMPOSITOR_GL_PROGRAM_FACTORY_H_
#define MIR_COMPOSITOR_GL_PROGRAM_FACTORY_H_

#include "mir/graphics/gl_program.h"
#include "mir/graphics/gl_texture_cache.h"
#include <memory>

namespace mir
{
namespace graphics
{

class GLProgramFactory
{
public:
    virtual ~GLProgramFactory() = default;

    virtual std::unique_ptr<GLProgram>
        create_gl_program(std::string const&, std::string const&) const = 0;
    virtual std::unique_ptr<GLTextureCache> create_texture_cache() const = 0;
protected:
    GLProgramFactory() = default;

private:
    GLProgramFactory(GLProgramFactory const&) = delete;
    GLProgramFactory& operator=(GLProgramFactory const&) = delete;

};

}
}

#endif /* MIR_COMPOSITOR_GL_PROGRAM_FACTORY_H_ */
