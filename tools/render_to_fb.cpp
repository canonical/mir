/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/display.h"

#include "mir_test/test_utils_graphics.h"

#define WIDTH 1280
#define HEIGHT 720

namespace mg=mir::graphics;
namespace mt=mir::test;

int main(int, char**)
{
    auto display = mg::create_display();

    mt::glAnimationBasic gl_animation;
    gl_animation.init_gl();

    for(;;)
    {
        gl_animation.render_gl();
        display->post_update();

        usleep(167);//60fps

        gl_animation.step();
    }

    return 0;
}

