/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/gl_program_factory.h"
#include "overlay_gl_compositor.h"

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace
{
std::string const vertex_shader{};
std::string const fragment_shader{};
}
mga::OverlayGLProgram::OverlayGLProgram(GLProgramFactory const& factory) :
    overlay_program{factory.create_gl_program(vertex_shader, fragment_shader)}
{
}
