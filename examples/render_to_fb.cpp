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
#include "mir/graphics/display_buffer.h"

#include "mir/draw/graphics.h"

#include <unistd.h>

#define WIDTH 1280
#define HEIGHT 720

namespace mg=mir::graphics;

int main(int, char**)
{
    auto platform = mg::create_platform();
    auto display = platform->create_display();

    mir::draw::glAnimationBasic gl_animation;
    gl_animation.init_gl();

    for(;;)
    {
        display->for_each_display_buffer([&](mg::DisplayBuffer& buffer)
        {
            buffer.make_current();
            buffer.clear();

            gl_animation.render_gl();

            buffer.post_update();
        });

        usleep(167);//60fps
        gl_animation.step();
    }

    return 0;
}

