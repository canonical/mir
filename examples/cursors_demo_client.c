/*
 * Copyright © 2014 Canonical LTD
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
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "eglapp.h"
#include <stdio.h>
#include <unistd.h>
#include <GLES2/gl2.h>

void configure_cursor(MirSurface *surface, unsigned int cursor_index)
{
    char const *const cursors[] = {
        mir_arrow_cursor_name,
        mir_busy_cursor_name,
        mir_caret_cursor_name,
        mir_pointing_hand_cursor_name,
        mir_open_hand_cursor_name,
        mir_closed_hand_cursor_name,
        mir_horizontal_resize_cursor_name,
        mir_vertical_resize_cursor_name,
        mir_diagonal_resize_bottom_to_top_cursor_name,
        mir_diagonal_resize_top_to_bottom_cursor_name,
        mir_omnidirectional_resize_cursor_name,
        mir_vsplit_resize_cursor_name,
        mir_hsplit_resize_cursor_name,
        mir_crosshair_cursor_name
    };

    size_t num_cursors = sizeof(cursors)/sizeof(*cursors);
    size_t real_index = cursor_index % num_cursors;
    
    MirCursorConfiguration *conf = mir_cursor_configuration_from_name(cursors[real_index]);

    mir_wait_for(mir_surface_configure_cursor(surface, conf));
    
    mir_cursor_configuration_destroy(conf);
}

int main(int argc, char *argv[])
{
    unsigned int width = 128, height = 128;

    if (!mir_eglapp_init(argc, argv, &width, &height))
        return 1;

    glClearColor(0.5, 0.5, 0.5, mir_eglapp_background_opacity);
    glClear(GL_COLOR_BUFFER_BIT);
    mir_eglapp_swap_buffers();

    unsigned int cursor_index = 0;
    while (mir_eglapp_running())
    {
        configure_cursor(mir_eglapp_native_surface(), cursor_index++);
        sleep(1);
    }

    mir_eglapp_shutdown();

    return 0;
}
