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
#include "mir/graphics/gl_context.h"
#include "overlay_gl_compositor.h"

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace
{
std::string const vertex_shader
{
    "attribute vec4 position;\n"
    "void main() {\n"
    "   gl_Position = position;\n"
    "}\n"
};

std::string const fragment_shader
{
    "precision mediump float;\n"
    "void main() {\n"
    "   gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "}\n"
};
}
mga::OverlayGLProgram::OverlayGLProgram(
    GLProgramFactory const& factory, GLContext const& context)
{
    context.make_current();
    overlay_program = factory.create_gl_program(vertex_shader, fragment_shader);
    context.release_current();
}

void mga::OverlayGLProgram::render(RenderableList const&, SwappingGLContext const&) const
{
}
