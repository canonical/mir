/*
 * Trivial GL demo; flashes the screen. Showing how simple life is with eglapp.
 *
 * Copyright © 2013 Canonical Ltd.
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "eglapp.h"
#include <stdio.h>
#include <unistd.h>
#include <GLES2/gl2.h>

typedef struct Color
{
    GLfloat r, g, b, a;
} Color;

int main(int argc, char *argv[])
{
    unsigned int width = 0, height = 0;

    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
        return 1;

    float const opacity = mir_eglapp_background_opacity;
    Color red = {opacity, 0.0f, 0.0f, opacity};
    Color green = {0.0f, opacity, 0.0f, opacity};
    Color blue = {0.0f, 0.0f, opacity, opacity};

    /* This is probably the simplest GL you can do */
    while (mir_eglapp_running())
    {
        glClearColor(red.r, red.g, red.b, red.a);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);

        glClearColor(green.r, green.g, green.b, green.a);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);

        glClearColor(blue.r, blue.g, blue.b, blue.a);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(1);
    }

    mir_eglapp_cleanup();

    return 0;
}
