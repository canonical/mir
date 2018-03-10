/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */
#ifndef MIR_DRAW_GRAPHICS
#define MIR_DRAW_GRAPHICS

#include <GLES2/gl2.h>

namespace mir
{
namespace draw
{

class glAnimationBasic
{
public:
    glAnimationBasic();

    void init_gl();
    void render_gl();
    void step();
    int texture_width();
    int texture_height();

private:
    GLuint program, vPositionAttr, uvCoord, slideUniform;
    float slide;
};

}
}

#endif /* MIR_DRAW_GRAPHICS */
