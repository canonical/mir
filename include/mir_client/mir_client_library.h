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

#include <EGL/eglplatform.h>

/* This header defines the MIR client library's C API.
 *
 * This API comprises a suite of free functions. The functions are asynchronous:
 * callers must pass in a callback function to determine when the call completes,
 * and should check any object returned by the callback is valid before use.
 */

/* Display server connection API */
typedef struct MirConnection MirConnection;
typedef struct MirWaitHandle MirWaitHandle;
typedef struct MirSurface MirSurface;

typedef void (*mir_connected_callback)(MirConnection *connection, void *client_context);
typedef void (*mir_surface_lifecycle_callback)(MirSurface *surface, void *client_context);

/* Surface API */
typedef enum MirPixelFormat
{
    mir_pixel_format_rgba_8888
} MirPixelFormat;

typedef struct MirSurfaceParameters
{
    char const *name;
    int width;
    int height;
    MirPixelFormat pixel_format;
} MirSurfaceParameters;

enum { mir_platform_package_max = 32 };

typedef struct MirPlatformPackage
{
    int data_items;
    int fd_items;

    int data[mir_platform_package_max];
    int fd[mir_platform_package_max];
} MirPlatformPackage;

enum { mir_buffer_package_max = 32 };

typedef struct MirBufferPackage
{
    int data_items;
    int fd_items;

    int data[mir_buffer_package_max];
    int fd[mir_buffer_package_max];

    int stride;
} MirBufferPackage;

typedef struct MirGraphicsRegion
{
    int width;
    int height;
    int stride;
    MirPixelFormat pixel_format;
    char *vaddr;

} MirGraphicsRegion;

typedef struct MirDisplayInfo
{
    int width;
    int height;
} MirDisplayInfo;

/*
 * Request a connection to the MIR server.
 * The supplied callback is called when the connection is established, or fails.
 */
MirWaitHandle* mir_connect(
    char const *socket_file,
    char const *app_name,
    mir_connected_callback callback,
    void *client_context);

/* Return a non-zero value if the supplied connection is valid, 0 otherwise. */
int mir_connection_is_valid(MirConnection *connection);

/*
 * Returns a text description of any error resulting in an invalid connection,
 * or the empty string "" if the connection is valid.
 */
char const *mir_connection_get_error_message(MirConnection *connection);

/* Release a connection to the MIR server. */
void mir_connection_release(MirConnection *connection);

void mir_connection_get_platform(MirConnection *connection, MirPlatformPackage *platform_package);

void mir_connection_get_display_info(MirConnection *connection, MirDisplayInfo *display_info);

/* Request a new MIR surface on the supplied connection with the supplied parameters. */
MirWaitHandle* mir_surface_create(
    MirConnection *connection,
    MirSurfaceParameters const *surface_parameters,
    mir_surface_lifecycle_callback callback,
    void *client_context);

/* returns an EGLNativeWindowType for a surface that the client can use for 
   OpenGL ES 2.0 acceleration */
EGLNativeWindowType mir_get_egl_type(MirSurface *surface);

/* Return a non-zero value if the supplied connection is valid, 0 otherwise. */
int mir_surface_is_valid(MirSurface *surface);

/* Returns a text description of any error resulting in an invalid surface,
   or the empty string "" if the surface is valid. */
char const *mir_surface_get_error_message(MirSurface *surface);

/* Get a valid surface's parameters. */
void mir_surface_get_parameters(MirSurface *surface, MirSurfaceParameters *parameters);

/* Get a surface's buffer. */
void mir_surface_get_current_buffer(MirSurface *surface, MirBufferPackage *buffer_package);

/* Get a surface's graphics_region. */
void mir_surface_get_graphics_region(
    MirSurface *surface,
    MirGraphicsRegion *graphics_region);

/* Advance a surface's buffer. */
MirWaitHandle* mir_surface_next_buffer(
    MirSurface *surface,
    mir_surface_lifecycle_callback callback,
    void *context);

/* Release the supplied surface and any associated buffer. */
MirWaitHandle* mir_surface_release(
    MirSurface *surface,
    mir_surface_lifecycle_callback callback,
    void *context);

void mir_wait_for(MirWaitHandle* wait_handle);

/* Return the id of the surface. (Only useful for debug output.) */
int mir_debug_surface_id(MirSurface *surface);

#ifdef __cplusplus
}
#endif

#endif /* MIR_CLIENT_LIBRARY_H */
