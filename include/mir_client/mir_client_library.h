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

#ifndef MIR_CLIENT_LIBRARY_H
#define MIR_CLIENT_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

/* This header defines the MIR client library's C API.

   This API comprises a suite of free functions. The functions are asynchronous:
   callers must pass in a callback function to determine when the call completes,
   and should check any object returned by the callback is valid before use.
 */

/* Display server connection API */
typedef struct MirConnection MirConnection;

typedef void (* mir_connected_callback)(MirConnection * connection, void * client_context);
typedef void (* mir_disconnected_callback)(void * client_context);

/* Request a connection to the MIR server.
   The supplied callback is called when the connection is established, or fails.
*/
void mir_connect(mir_connected_callback callback, void * client_context);

/* Return a non-zero value if the supplied connection is valid, 0 otherwise. */
int mir_connection_is_valid(MirConnection * connection);

/* Returns a text description of any error resulting in an invalid connection,
   or the empty string "" if the connection is valid. */
char const * mir_connection_get_error_message(MirConnection * connection);

/* Release a connection to the MIR server.
 */
void mir_connection_release(MirConnection * connection);

/* Surface API */
typedef enum MirPixelFormat
{
    mir_pixel_format_rgba_8888
} MirPixelFormat;

typedef struct MirSurfaceParameters
{
    int width;
    int height;
    MirPixelFormat pixel_format;
} MirSurfaceParameters;

typedef struct MirSurface MirSurface;

typedef void (* mir_surface_lifecycle_callback)(MirSurface * surface, void * client_context);

/* Request a new MIR surface on the supplied connection with the supplied parameters. */
void mir_surface_create(MirConnection * connection,
                        MirSurfaceParameters const * surface_parameters,
                        mir_surface_lifecycle_callback callback,
                        void * client_context);

/* Return a non-zero value if the supplied connection is valid, 0 otherwise. */
int mir_surface_is_valid(MirSurface * surface);

/* Returns a text description of any error resulting in an invalid surface,
   or the empty string "" if the surface is valid. */
char const * mir_surface_get_error_message(MirSurface * surface);

/* Get a valid surface's parameters. */
MirSurfaceParameters mir_surface_get_parameters(MirSurface * surface);

/* Release the supplied surface and any associated buffer. */
void mir_surface_release(MirSurface * surface, mir_surface_lifecycle_callback callback, void * context);

/* Graphics buffer API */
typedef struct MirBuffer MirBuffer;

typedef void (* mir_buffer_advanced_callback)(MirBuffer * buffer, void * client_context);

/* Advance the surface's buffer. Calling this function releases any current
   surface buffer and registers a callback for the next buffer, when available.
 */
void mir_surface_advance_buffer(MirSurface * surface,
                                mir_buffer_advanced_callback callback,
                                void * client_context);

int mir_buffer_is_valid(MirBuffer * buffer);

/* Returns a text description of any error resulting in an invalid buffer,
   or the empty string "" if the buffer is valid. */
char const * mir_buffer_get_error_message(MirBuffer * buffer);

/* Returns the number of microseconds, from now, before the next vblank
   event. */
int mir_buffer_get_next_vblank_microseconds(MirBuffer * buffer);

/* Return the id of the surface. (Only useful for debug output.) */
int mir_debug_surface_id(MirSurface * surface);


#ifdef __cplusplus
}
#endif

#endif /* MIR_CLIENT_LIBRARY_H */
