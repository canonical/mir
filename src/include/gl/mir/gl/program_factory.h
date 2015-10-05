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

#ifndef MIR_GL_PROGRAM_FACTORY_H_
#define MIR_GL_PROGRAM_FACTORY_H_

#include "program.h"
#include "texture_cache.h"
#include <memory>

namespace mir
{
namespace gl
{

class ProgramFactory
{
public:
    virtual ~ProgramFactory() = default;

    virtual std::unique_ptr<Program>
        create_gl_program(std::string const&, std::string const&) const = 0;
    virtual std::unique_ptr<TextureCache> create_texture_cache() const = 0;
protected:
    ProgramFactory() = default;

private:
    ProgramFactory(ProgramFactory const&) = delete;
    ProgramFactory& operator=(ProgramFactory const&) = delete;

};

}
}

#endif /* MIR_GL_PROGRAM_FACTORY_H_ */
