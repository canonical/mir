/*
 * Trivial GL demo; switches surface states. Showing how simple life is with eglapp.
 *
 * Copyright © 2014 Canonical Ltd.
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
 * Author: Alan Griffiths <alan@octopull.co.uk>
 */

#include "eglapp.h"
#include <mir_toolkit/mir_client_library.h>

#include <stdio.h>
#include <unistd.h>
#include <GLES2/gl2.h>

static void toggle_surface_state(MirWindow* const surface, MirWindowState* state);

typedef struct Color
{
    GLfloat r, g, b, a;
} Color;

int main(int argc, char *argv[])
{
    float const opacity = mir_eglapp_background_opacity;
    Color const orange = {0.866666667f, 0.282352941f, 0.141414141f, opacity};

    unsigned int width = 120, height = 120;

    if (!mir_eglapp_init(argc, argv, &width, &height, NULL))
        return 1;

    MirWindow* const window = mir_eglapp_native_window();
    MirWindowState state = mir_window_get_state(window);

    /* This is probably the simplest GL you can do */
    while (mir_eglapp_running())
    {
        glClearColor(orange.r, orange.g, orange.b, orange.a);
        glClear(GL_COLOR_BUFFER_BIT);
        mir_eglapp_swap_buffers();
        sleep(2);

        toggle_surface_state(window, &state);
    }

    mir_eglapp_cleanup();

    return 0;
}

#define NEW_STATE(new_state)\
        puts("Requesting state: " #new_state);\
        *state = new_state

#define PRINT_STATE(state)\
    case state:\
        puts("Current state: " #state);\
        break

void toggle_surface_state(MirWindow* const surface, MirWindowState* state)
{
    switch (mir_window_get_state(surface))
    {
    PRINT_STATE(mir_window_state_unknown);
    PRINT_STATE(mir_window_state_restored);
    PRINT_STATE(mir_window_state_minimized);
    PRINT_STATE(mir_window_state_fullscreen);
    PRINT_STATE(mir_window_state_maximized);
    PRINT_STATE(mir_window_state_vertmaximized);
    PRINT_STATE(mir_window_state_horizmaximized);
    default:
        puts("Current state: unknown");
    }

    switch (*state)
    {
    case mir_window_state_restored:
        NEW_STATE(mir_window_state_unknown);
        break;

    case mir_window_state_minimized:
        NEW_STATE(mir_window_state_restored);
        break;

    case mir_window_state_fullscreen:
        NEW_STATE(mir_window_state_vertmaximized);
        break;

    case mir_window_state_maximized:
        NEW_STATE(mir_window_state_fullscreen);
        break;

    case mir_window_state_vertmaximized:
        NEW_STATE(mir_window_state_horizmaximized);
        break;

    case mir_window_state_horizmaximized:
        NEW_STATE(mir_window_state_minimized);
        break;

    default:
        NEW_STATE(mir_window_state_maximized);
        break;
    }
    mir_window_set_state(surface, *state);
}
