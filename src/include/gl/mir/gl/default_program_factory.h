/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GL_DEFAULT_PROGRAM_FACTORY_H_
#define MIR_GL_DEFAULT_PROGRAM_FACTORY_H_

#include "program_factory.h"
#include <mutex>

namespace mir
{
namespace gl
{
class DefaultProgramFactory : public ProgramFactory
{
public:
    std::unique_ptr<Program> create_gl_program(std::string const&, std::string const&) const override;
    std::unique_ptr<TextureCache> create_texture_cache() const override;

private:
    /*
     * We need to serialize renderer creation because some GL calls used
     * during renderer construction that create unique resource ids
     * (e.g. glCreateProgram) are not thread-safe when the threads are
     * have the same or shared EGL contexts.
     */
    std::mutex mutable mutex;
};
}
}

#endif /* MIR_GL_DEFAULT_PROGRAM_FACTORY_H_ */
