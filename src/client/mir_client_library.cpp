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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "mir_client/mir_client_library.h"

#include <cstddef>

void mir_connect(mir_connected_callback callback, void * context)
{
    callback(NULL, context);
}

int mir_connection_is_valid(MirConnection *)
{
    return 0;
}

char const * mir_connection_get_error_message(MirConnection *)
{
    return "not yet implemented!";
}

void mir_create_surface(MirConnection *,
                        MirSurfaceParameters const *,
                        mir_surface_created_callback callback,
                        void * context)
{
    callback(NULL, context);
}

int mir_surface_is_valid(MirSurface *)
{
    return 0;
}

char const * mir_surface_get_error_message(MirSurface *)
{
    return "not yet implemented!";
}

MirSurfaceParameters mir_surface_get_parameters(MirSurface *)
{
    return MirSurfaceParameters{0, 0, mir_pixel_format_rgba_8888};
}

void mir_surface_release(MirSurface *)
{
}

void mir_advance_buffer(MirSurface * surface,
                        mir_buffer_advanced_callback callback,
                        void * context);

int mir_buffer_is_valid(MirBuffer *)
{
    return 0;
}

char const * mir_buffer_get_error_message(MirBuffer *)
{
    return "not yet implemented!";
}

int mir_buffer_get_next_vblank_microseconds(MirBuffer *)
{
    return -1;
}
