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

#include "gl_renderer_factory.h"
#include "mir/compositor/gl_renderer.h"
#include "mir/graphics/gl_program.h"
#include "mir/geometry/rectangle.h"

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

std::unique_ptr<mg::GLProgram>
mg::ProgramFactory::create_gl_program(
    std::string const& vertex_shader,
    std::string const& fragment_shader) const
{
    std::lock_guard<decltype(mutex)> lock(mutex);
    return std::unique_ptr<mg::GLProgram>(new GLProgram(vertex_shader.c_str(), fragment_shader.c_str()));
}

mc::GLRendererFactory::GLRendererFactory(std::shared_ptr<mg::GLProgramFactory> const& factory) :
    program_factory(factory)
{
}

std::unique_ptr<mc::Renderer>
mc::GLRendererFactory::create_renderer_for(geom::Rectangle const& rect)
{
    auto raw = new GLRenderer(*program_factory, rect);
    return std::unique_ptr<mc::Renderer>(raw);
}
