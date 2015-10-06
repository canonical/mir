
/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/gl/default_program_factory.h"
#include "mir/gl/program.h"
#include "recently_used_cache.h"

namespace mgl = mir::gl;

std::unique_ptr<mgl::Program>
mgl::DefaultProgramFactory::create_gl_program(
    std::string const& vertex_shader,
    std::string const& fragment_shader) const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return std::make_unique<SimpleProgram>(
        vertex_shader.c_str(), fragment_shader.c_str());
}

std::unique_ptr<mgl::TextureCache> mgl::DefaultProgramFactory::create_texture_cache() const
{
    return std::make_unique<RecentlyUsedCache>();
}
