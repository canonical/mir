/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois   <kevin.dubois@canonical.com>
 */

#include "eglapp.h"
#include "graphics.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[])
try
{
    unsigned int width = 640, height = 480;
    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
        return 1;

    mir::draw::glAnimationBasic gl_animation;
    gl_animation.init_gl();

    while (mir_eglapp_running())
    {
        gl_animation.render_gl();
        mir_eglapp_swap_buffers();
        gl_animation.step();
    }

    mir_eglapp_cleanup();
    return 0;
}
catch(std::exception& e)
{
    std::cerr << "error : " << e.what() << std::endl;
    return 1;
}
