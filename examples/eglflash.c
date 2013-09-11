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
#define _POSIX_C_SOURCE 199309L
#include "eglapp.h"
#include <stdio.h>
#include <unistd.h>
#include <GLES2/gl2.h>
#include <time.h>

int main(int argc, char *argv[])
{
    unsigned int width = 0, height = 0;

    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    if (!mir_eglapp_make_current())
        return 1;

    struct timespec ts =
    {
        0,
        1000000000 / 2
    };

    /* This is probably the simplest GL you can do */
    while (mir_eglapp_running())
    {
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        nanosleep(&ts, NULL);

        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        nanosleep(&ts, NULL);

        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        nanosleep(&ts, NULL);

        ts.tv_nsec = 0;
    }

    mir_eglapp_shutdown();

    return 0;
}
