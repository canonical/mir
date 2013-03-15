/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MIR_CLIENT_LIBRARY_H
#define MIR_CLIENT_LIBRARY_H

#ifdef __cplusplus
/** The C client API
 */
namespace mir_toolkit {
extern "C" {
#endif

/* This header defines the MIR client library's C API. */

/* Display server connection API */
typedef void* MirEGLNativeWindowType;
typedef void* MirEGLNativeDisplayType;
typedef struct MirConnection MirConnection;
typedef struct MirSurface MirSurface;

/**
 * Returned by asynchronous functions. Must not be free'd by
 * callers. See the individual function documentation for information
 * on the lifetime of wait handles.
 */
typedef struct MirWaitHandle MirWaitHandle;

/**
 * Callback to be passed when issuing a mir_connect request.
 *   \param [in] connection          the new connection
 *   \param [in,out] client_context  context provided by client in calling
 *                                   mir_connect
 */
typedef void (*mir_connected_callback)(MirConnection *connection, void *client_context);

/**
 * Callback to be passed when calling:
 *  - mir_surface_create
 *  - mir_surface_next_buffer
 *  - mir_surface_release
 *   \param [in] surface             the surface being updated
 *   \param [in,out] client_context  context provided by client in calling
 *                                   mir_connect
 */
typedef void (*mir_surface_lifecycle_callback)(MirSurface *surface, void *client_context);

/**
 * The order of components in a format enum matches the
 * order of the components as they would be written in an
 *  integer representing a pixel value of that format.
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

/** TODO */
typedef enum MirBufferUsage
{
    mir_buffer_usage_hardware = 1,
    mir_buffer_usage_software
} MirBufferUsage;

/** TODO */
typedef struct MirSurfaceParameters
{
    char const *name;
    int width;
    int height;
    MirPixelFormat pixel_format;
    MirBufferUsage buffer_usage;
} MirSurfaceParameters;

enum { mir_platform_package_max = 32 };

/** TODO */
typedef struct MirPlatformPackage
{
    int data_items;
    int fd_items;

    int data[mir_platform_package_max];
    int fd[mir_platform_package_max];
} MirPlatformPackage;

enum { mir_buffer_package_max = 32 };

/** TODO */
typedef struct MirBufferPackage
{
    int data_items;
    int fd_items;

    int data[mir_buffer_package_max];
    int fd[mir_buffer_package_max];

    int stride;
} MirBufferPackage;

/** TODO */
typedef struct MirGraphicsRegion
{
    int width;
    int height;
    int stride;
    MirPixelFormat pixel_format;
    char *vaddr;

} MirGraphicsRegion;

enum { mir_supported_pixel_format_max = 32 };

/** TODO */
typedef struct MirDisplayInfo
{
    int width;
    int height;
    int supported_pixel_format_items;
    MirPixelFormat supported_pixel_format[mir_supported_pixel_format_max];
} MirDisplayInfo;

/**
 * Request a connection to the MIR server. The supplied callback is
 * called when the connection is established, or fails. The returned
 * wait handle remains valid until the connection has been released.
 *   \param [in] server              a name identifying the server
 *   \param [in] app_name            a name referring to the application
 *   \param [in] callback            callback function to be invoked when request
 *                                   completes
 *   \param [in,out] client_context  passed to the callback function
 *   \return                         a handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_connect(
    char const *server,
    char const *app_name,
    mir_connected_callback callback,
    void *client_context);

/**
 * Test for a valid connection
 * \param [in] connection  the connection
 * \return                 a non-zero value if the supplied connection is
 *                         valid, 0 otherwise
 */
int mir_connection_is_valid(MirConnection *connection);

/**
 * Retrieve a text description of the last error. The returned string is owned
 * by the library and remains valid until the connection has been released.
 *   \param [in] connection  the connection
 *   \return                 a text description of any error resulting in an
 *                           invalid connection, or the empty string "" if the
 *                           connection is valid.
 */
char const *mir_connection_get_error_message(MirConnection *connection);

/**
 * Release a connection to the MIR server
 *   \param [in] connection  the connection
 */
void mir_connection_release(MirConnection *connection);

/**
 * Query platform specific data and/or fd's that are required to initialize
 * gl/egl bits.
 *   \param [in]  connection        the connection
 *   \param [out] platform_package  structure to be populated
 */
void mir_connection_get_platform(MirConnection *connection, MirPlatformPackage *platform_package);

/**
 * Query the display
 *   \param [in]  connection    the connection
 *   \param [out] display_info  structure to be populated
 */
void mir_connection_get_display_info(MirConnection *connection, MirDisplayInfo *display_info);

/**
 * Get a display type that can be used for OpenGL ES 2.0 acceleration.
 *   \param [in] connection  the connection
 *   \return                 an EGLNativeDisplayType that the client can use
 */
MirEGLNativeDisplayType mir_connection_get_egl_native_display(MirConnection *connection);

/**
 * Request a new MIR surface on the supplied connection with the
 * supplied parameters. The returned handle remains valid until
 * the surface has been released.
 *   \param [in] connection          the connection
 *   \param [in] surface_parameters  request surface parameters
 *   \param [in] callback            callback function to be invoked when
 *                                   request completes
 *   \param [in,out] client_context  passed to the callback function
 *   \return                         a handle that can be passed to
 *                                   mir_wait_for
 */
MirWaitHandle *mir_surface_create(
    MirConnection *connection,
    MirSurfaceParameters const *surface_parameters,
    mir_surface_lifecycle_callback callback,
    void *client_context);

/**
 * Get a window type that can be used for OpenGL ES 2.0 acceleration
 *   \param [in] surface  the surface
 *   \return              an EGLNativeWindowType that the client can use
 */
MirEGLNativeWindowType mir_surface_get_egl_native_window(MirSurface *surface);

/**
 * Test for a valid surface
 *   \param [in] surface  the surface
 *   \return              a non-zero value if the supplied surface is valid,
 *                        0 otherwise
 */
int mir_surface_is_valid(MirSurface *surface);

/**
 * Retrieve a text description of the error. The returned string is owned
 * by the library and remains valid until the surface or the associated
 * connection has been released.
 *   \param [in] surface  the surface
 *   \return              a text description of any error resulting in an
 *                        invalid surface, or the empty string "" if the
 *                        connection is valid.
 */
char const *mir_surface_get_error_message(MirSurface *surface);

/**
 * Get a surface's parameters.
 *   \pre                     the surface is valid
 *   \param [in] surface      the surface
 *   \param [out] parameters  structure to be populated
 */
void mir_surface_get_parameters(MirSurface *surface, MirSurfaceParameters *parameters);

/**
 * Get a surface's buffer in "raw" representation
 *   \pre                         the surface is valid
 *   \param [in] surface          the surface
 *   \param [out] buffer_package  structure to be populated
 */
void mir_surface_get_current_buffer(MirSurface *surface, MirBufferPackage *buffer_package);

/**
 * Get a surface's graphics_region, i.e., map the graphics buffer to main
 * memory.
 *   \pre                         the surface is valid
 *   \param [in] surface          the surface
 *   \param [out] graphics_region structure to be populated
 */
void mir_surface_get_graphics_region(
    MirSurface *surface,
    MirGraphicsRegion *graphics_region);

/**
 * Advance a surface's buffer. The returned handle remains valid until
 * the next call to mir_surface_next_buffer, until the surface has
 * been released or the connection to the server has been released.
 *   \param [in] surface             the surface
 *   \param [in] callback            callback function to be invoked when
 *                                   request completes
 *   \param [in,out] client_context  passed to the callback function
 *   \return                         a handle that can be passed to
 *                                   mir_wait_for
 */
MirWaitHandle *mir_surface_next_buffer(
    MirSurface *surface,
    mir_surface_lifecycle_callback callback,
    void *client_context);

/**
 * Release the supplied surface and any associated buffer. The
 * returned wait handle remains valid until the connection to the
 * server is released.
 *   \param [in] surface             the surface
 *   \param [in] callback            callback function to be invoked when
 *                                   request completes
 *   \param [in,out] client_context  passed to the callback function
 *   \return                         a handle that can be passed to
 *                                   mir_wait_for
 */
MirWaitHandle *mir_surface_release(
    MirSurface *surface,
    mir_surface_lifecycle_callback callback,
    void *client_context);

/**
 * Wait on the supplied handle until the associated request has completed.
 *   \param [in] wait_handle  handle returned by an asynchronous request
 */
void mir_wait_for(MirWaitHandle *wait_handle);

/**
 * Return the id of the surface. (Only useful for debug output.)
 *   \param [in] surface  the surface
 *   \return              an internal id that identifies the surfaceS
 */
int mir_debug_surface_id(MirSurface *surface);

#ifdef __cplusplus
}
}
using namespace mir_toolkit;
#endif

#endif /* MIR_CLIENT_LIBRARY_H */
