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
 */

#ifndef MIR_CLIENT_LIBRARY_H
#define MIR_CLIENT_LIBRARY_H

#ifdef __cplusplus
extern "C" {
#endif

/* This header defines the MIR client library's C API. */

/* Display server connection API */
typedef void* MirEGLNativeWindowType;
typedef void* MirEGLNativeDisplayType;
typedef struct MirConnection MirConnection;
typedef struct MirSurface MirSurface;

/* Returned by asynchronous functions. Must not be free'd by
 * callers. See the individual function documentation for information
 * on the lifetime of wait handles.
 */
typedef struct MirWaitHandle MirWaitHandle;

/* Callback to be passed when issuing a connect request. The new
 * connection is passed as an argument.
 */
typedef void (*mir_connected_callback)(MirConnection *connection, void *client_context);

/* Callback to be passed when a request for:
 *   - creating a surface
 *   - advancing a buffer
 *   - destroying a surface
 * is issued.
 */
typedef void (*mir_surface_lifecycle_callback)(MirSurface *surface, void *client_context);

/* Surface API */
/*
 * The order of components in a format enum matches the
 * order of the components as they would be written in an
 * integer representing a pixel value of that format.
 *
 * For example, abgr_8888 corresponds to 0xAABBGGRR, which will
 * end up as R,G,B,A in memory in a little endian system, and
 * as A,B,G,R in memory in a big endian system.
 */
typedef enum MirPixelFormat
{
    mir_pixel_format_invalid,
    mir_pixel_format_abgr_8888,
    mir_pixel_format_xbgr_8888,
    mir_pixel_format_argb_8888,
    mir_pixel_format_xrgb_8888,
    mir_pixel_format_bgr_888
} MirPixelFormat;

typedef enum MirBufferUsage
{
    mir_buffer_usage_hardware = 1,
    mir_buffer_usage_software
} MirBufferUsage;

typedef struct MirSurfaceParameters
{
    char const *name;
    int width;
    int height;
    MirPixelFormat pixel_format;
    MirBufferUsage buffer_usage;
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

enum { mir_supported_pixel_format_max = 32 };

typedef struct MirDisplayInfo
{
    int width;
    int height;
    int supported_pixel_format_items;
    MirPixelFormat supported_pixel_format[mir_supported_pixel_format_max];
} MirDisplayInfo;

/*
 * Request a connection to the MIR server. The supplied callback is
 * called when the connection is established, or fails. The returned
 * handle remains valid until the connection has been released.
 */
MirWaitHandle *mir_connect(
    char const *socket_file,
    char const *app_name,
    mir_connected_callback callback,
    void *client_context);

/* Return a non-zero value if the supplied connection is valid, 0 otherwise. */
int mir_connection_is_valid(MirConnection *connection);

/*
 * Returns a text description of any error resulting in an invalid
 * connection, or the empty string "" if the connection is valid. The
 * returned string is owned by the library and remains valid until the
 * connection has been released.
 */
char const *mir_connection_get_error_message(MirConnection *connection);

/* Release a connection to the MIR server. */
void mir_connection_release(MirConnection *connection);

/* Query platform specific data and/or fd's that are required to initialize gl/egl bits. */
void mir_connection_get_platform(MirConnection *connection, MirPlatformPackage *platform_package);

/* Query the width and height of the primary display */
void mir_connection_get_display_info(MirConnection *connection, MirDisplayInfo *display_info);

/*
 * Returns an EGLNativeDisplayType that the client can use for
 * OpenGL ES 2.0 acceleration.
 */
MirEGLNativeDisplayType mir_connection_get_egl_native_display(MirConnection *connection);

/* Request a new MIR surface on the supplied connection with the
 * supplied parameters. The returned handle remains valid until
 * the surface has been released.
 */
MirWaitHandle *mir_surface_create(
    MirConnection *connection,
    MirSurfaceParameters const *surface_parameters,
    mir_surface_lifecycle_callback callback,
    void *client_context);

/* returns an EGLNativeWindowType for a surface that the client can use for
   OpenGL ES 2.0 acceleration */
MirEGLNativeWindowType mir_surface_get_egl_native_window(MirSurface *surface);

/* Return a non-zero value if the supplied connection is valid, 0 otherwise. */
int mir_surface_is_valid(MirSurface *surface);

/* Returns a text description of any error resulting in an invalid
 * surface, or the empty string "" if the surface is valid. The
 * returned string is owned by the library remains valid until the
 * surface or the associated connection is released.
 */
char const *mir_surface_get_error_message(MirSurface *surface);

/* Get a valid surface's parameters. */
void mir_surface_get_parameters(MirSurface *surface, MirSurfaceParameters *parameters);

/* Get a surface's buffer in "raw" representation. */
void mir_surface_get_current_buffer(MirSurface *surface, MirBufferPackage *buffer_package);

/* Get a surface's graphics_region, i.e., map the graphics buffer to main memory. */
void mir_surface_get_graphics_region(
    MirSurface *surface,
    MirGraphicsRegion *graphics_region);

/* Advance a surface's buffer. The returned handle remains valid until
 * the next call to mir_surface_next_buffer, until the surface has
 * been released or the connection to the server has been released.
 */
MirWaitHandle *mir_surface_next_buffer(
    MirSurface *surface,
    mir_surface_lifecycle_callback callback,
    void *context);

/* Release the supplied surface and any associated buffer. The
 * returned wait handle remains valid until the connection to the
 * server is released.
 */
MirWaitHandle *mir_surface_release(
    MirSurface *surface,
    mir_surface_lifecycle_callback callback,
    void *context);

/* Wait on the supplied handle until the associated request has completed. */
void mir_wait_for(MirWaitHandle *wait_handle);

/* Return the id of the surface. (Only useful for debug output.) */
int mir_debug_surface_id(MirSurface *surface);

#ifdef __cplusplus
}
#endif

#endif /* MIR_CLIENT_LIBRARY_H */
