/*
 * Trivial GL demo; flashes the screen. Showing how simple life is with eglapp.
 *
 * Copyright © 2013 Canonical Ltd.
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "./eglapp.h"
#include <stdio.h> /* sleep() */
#include <unistd.h> /* sleep() */
#include <GLES2/gl2.h>

int main(int argc, char *argv[])
{
    int width = 0, height = 0;  /* Use the full display */

    (void)argc;
    (void)argv;

    if (!mir_egl_app_init(&width, &height))
    {
        printf("Can't initialize EGL\n");
        return 1;
    }

    /* This is probably the simplest GL you can do */
    while (1)
    {
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_egl_swap_buffers();
        sleep(1);

        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_egl_swap_buffers();
        sleep(1);

        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_egl_swap_buffers();
        sleep(1);
    }

    return 0;
}
