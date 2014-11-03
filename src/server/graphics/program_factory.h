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

#ifndef MIR_GRAPHICS_GL_RENDERER_FACTORY_H_
#define MIR_GRAPHICS_GL_RENDERER_FACTORY_H_

#include "mir/graphics/gl_program_factory.h"
#include "mir/graphics/gl_program.h"
#include "mir/graphics/gl_texture_cache.h"
#include <mutex>

namespace mir
{
namespace graphics
{
class ProgramFactory : public GLProgramFactory
{
public:
    std::unique_ptr<GLProgram> create_gl_program(std::string const&, std::string const&) const override;
    std::unique_ptr<GLTextureCache> create_texture_cache() const override;

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

#endif /* MIR_GRAPHICS_GL_RENDERER_FACTORY_H_ */
