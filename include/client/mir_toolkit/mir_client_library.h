/*
 * Copyright © 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MIR_CLIENT_LIBRARY_H
#define MIR_CLIENT_LIBRARY_H

#include <mir_toolkit/mir_native_buffer.h>
#include <mir_toolkit/client_types.h>
#include <mir_toolkit/common.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/* This header defines the Mir client library's C API. */

/**
 * Request a connection to the Mir server. The supplied callback is called when
 * the connection is established, or fails. The returned wait handle remains
 * valid until the connection has been released.
 *   \warning callback could be called from another thread. You must do any
 *            locking appropriate to protect your data accessed in the
 *            callback.
 *   \param [in] server       File path of the server socket to connect to, or
 *                            NULL to choose the default server
 *   \param [in] app_name     A name referring to the application
 *   \param [in] callback     Callback function to be invoked when request
 *                            completes
 *   \param [in,out] context  User data passed to the callback function
 *   \return                  A handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_connect(
    char const *server,
    char const *app_name,
    mir_connected_callback callback,
    void *context);

/**
 * Perform a mir_connect() but also wait for and return the result.
 *   \param [in] server    File path of the server socket to connect to, or
 *                         NULL to choose the default server
 *   \param [in] app_name  A name referring to the application
 *   \return               The resulting MirConnection
 */
MirConnection *mir_connect_sync(char const *server, char const *app_name);

/**
 * Test for a valid connection
 * \param [in] connection  The connection
 * \return                 A non-zero value if the supplied connection is
 *                         valid, 0 otherwise
 */
int mir_connection_is_valid(MirConnection *connection);

/**
 * Retrieve a text description of the last error. The returned string is owned
 * by the library and remains valid until the connection has been released.
 *   \param [in] connection  The connection
 *   \return                 A text description of any error resulting in an
 *                           invalid connection, or the empty string "" if the
 *                           connection is valid.
 */
char const *mir_connection_get_error_message(MirConnection *connection);

/**
 * Release a connection to the Mir server
 *   \param [in] connection  The connection
 */
void mir_connection_release(MirConnection *connection);

/**
 * Query platform-specific data and/or file descriptors that are required to
 * initialize GL/EGL features.
 *   \param [in]  connection        The connection
 *   \param [out] platform_package  Structure to be populated
 */
void mir_connection_get_platform(MirConnection *connection, MirPlatformPackage *platform_package);

/**
 * Register a callback to be called when a Lifecycle state change occurs.
 *   \param [in] connection     The connection
 *   \param [in] callback       The function to be called when the state change occurs
 *   \param [in,out] context    User data passed to the callback function
 */
void mir_connection_set_lifecycle_event_callback(MirConnection* connection,
    mir_lifecycle_event_callback callback, void* context);

/** 
 * \deprecated Use mir_connection_create_display_config
 */
__attribute__((__deprecated__("Use mir_connection_create_display_config()")))
void mir_connection_get_display_info(MirConnection *connection, MirDisplayInfo *display_info);

/**
 * Query the display
 *   \warning return value must be destroyed via mir_display_config_destroy() 
 *   \warning may return null if connection is invalid 
 *   \param [in]  connection        The connection
 *   \return                        structure that describes the display configuration
 */
MirDisplayConfiguration* mir_connection_create_display_config(MirConnection *connection);

/**
 * Register a callback to be called when the hardware display configuration changes
 *
 * Once a change has occurred, you can use mir_connection_create_display_config to see
 * the new configuration.
 *
 *   \param [in] connection  The connection
 *   \param [in] callback     The function to be called when a display change occurs
 *   \param [in,out] context  User data passed to the callback function
 */
void mir_connection_set_display_config_change_callback(
    MirConnection* connection,
    mir_display_config_callback callback, void* context);

/**
 * Destroy the DisplayConfiguration resource acquired from mir_connection_create_display_config
 *   \param [in] display_configuration  The display_configuration information resource to be destroyed 
 */
void mir_display_config_destroy(MirDisplayConfiguration* display_configuration);

/**
 * Apply the display configuration
 *
 * The display configuration is applied to this connection only (per-connection
 * configuration) and is invalidated when a hardware change occurs. Clients should
 * register a callback with mir_connection_set_display_config_change_callback()
 * to get notified about hardware changes, so that the can apply a new configuration.
 *
 *   \warning This request may be denied. Check that the request succeeded with mir_connection_get_error_message.
 *   \param [in] connection             The connection
 *   \param [in] display_configuration  The display_configuration to apply 
 *   \return                            A handle that can be passed to mir_wait_for
 */
MirWaitHandle* mir_connection_apply_display_config(MirConnection *connection, MirDisplayConfiguration* display_configuration);

/**
 * Get a display type that can be used for OpenGL ES 2.0 acceleration.
 *   \param [in] connection  The connection
 *   \return                 An EGLNativeDisplayType that the client can use
 */
MirEGLNativeDisplayType mir_connection_get_egl_native_display(MirConnection *connection);

/**
 * Get the list of possible formats that a surface can be created with.
 *   \param [in] connection         The connection
 *   \param [out] formats           List of valid formats to create surfaces with 
 *   \param [in]  formats_size      size of formats list
 *   \param [out] num_valid_formats number of valid formats returned in formats
 */ 
void mir_connection_get_available_surface_formats(
    MirConnection* connection, MirPixelFormat* formats,
    unsigned const int format_size, unsigned int *num_valid_formats);

/**
 * Request a new Mir surface on the supplied connection with the supplied
 * parameters. The returned handle remains valid until the surface has been
 * released.
 *   \warning callback could be called from another thread. You must do any
 *            locking appropriate to protect your data accessed in the
 *            callback.
 *   \param [in] connection          The connection
 *   \param [in] surface_parameters  Request surface parameters
 *   \param [in] callback            Callback function to be invoked when
 *                                   request completes
 *   \param [in,out] context         User data passed to the callback function
 *   \return                         A handle that can be passed to
 *                                   mir_wait_for
 */
MirWaitHandle *mir_connection_create_surface(
    MirConnection *connection,
    MirSurfaceParameters const *surface_parameters,
    mir_surface_callback callback,
    void *context);

/**
 * Create a surface like in mir_connection_create_surface(), but also wait for
 * creation to complete and return the resulting surface.
 *   \param [in] connection  The connection
 *   \param [in] params      Parameters describing the desired surface
 *   \return                 The resulting surface
 */
MirSurface *mir_connection_create_surface_sync(
    MirConnection *connection,
    MirSurfaceParameters const *params);

/**
 * Set the event handler to be called when events arrive for a surface.
 *   \warning event_handler could be called from another thread. You must do
 *            and locking appropriate to protect your data accessed in the
 *            callback. There is also a chance that different events will be
 *            called back in different threads, for the same surface,
 *            simultaneously.
 *   \param [in] surface        The surface
 *   \param [in] event_handler  The event handler to call
 */
void mir_surface_set_event_handler(MirSurface *surface,
                                   MirEventDelegate const *event_handler);

/**
 * Get a window type that can be used for OpenGL ES 2.0 acceleration.
 *   \param [in] surface  The surface
 *   \return              An EGLNativeWindowType that the client can use
 */
MirEGLNativeWindowType mir_surface_get_egl_native_window(MirSurface *surface);

/**
 * Test for a valid surface
 *   \param [in] surface  The surface
 *   \return              A non-zero value if the supplied surface is valid,
 *                        0 otherwise
 */
int mir_surface_is_valid(MirSurface *surface);

/**
 * Retrieve a text description of the error. The returned string is owned by
 * the library and remains valid until the surface or the associated
 * connection has been released.
 *   \param [in] surface  The surface
 *   \return              A text description of any error resulting in an
 *                        invalid surface, or the empty string "" if the
 *                        connection is valid.
 */
char const *mir_surface_get_error_message(MirSurface *surface);

/**
 * Get a surface's parameters.
 *   \pre                     The surface is valid
 *   \param [in] surface      The surface
 *   \param [out] parameters  Structure to be populated
 */
void mir_surface_get_parameters(MirSurface *surface, MirSurfaceParameters *parameters);

/**
 * Get the underlying platform type so the buffer obtained in "raw" representation
 * in mir_surface_get_current_buffer() can be understood 
 *   \pre                     The surface is valid
 *   \param [in] surface      The surface
 *   \return                  One of mir_platform_type_android or mir_platform_type_gbm
 */
MirPlatformType mir_surface_get_platform_type(MirSurface *surface);

/**
 * Get a surface's buffer in "raw" representation.
 *   \pre                         The surface is valid
 *   \param [in] surface          The surface
 *   \param [out] buffer_package  Structure to be populated
 */
void mir_surface_get_current_buffer(MirSurface *surface, MirNativeBuffer **buffer_package);

/**
 * Get a surface's graphics_region, i.e., map the graphics buffer to main
 * memory.
 *   \pre                          The surface is valid
 *   \param [in] surface           The surface
 *   \param [out] graphics_region  Structure to be populated
 */
void mir_surface_get_graphics_region(
    MirSurface *surface,
    MirGraphicsRegion *graphics_region);

/**
 * Advance a surface's buffer. The returned handle remains valid until the next
 * call to mir_surface_swap_buffers, until the surface has been released or the
 * connection to the server has been released.
 *   \warning callback could be called from another thread. You must do any
 *            locking appropriate to protect your data accessed in the
 *            callback.
 *   \param [in] surface      The surface
 *   \param [in] callback     Callback function to be invoked when the request
 *                            completes
 *   \param [in,out] context  User data passed to the callback function
 *   \return                  A handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_surface_swap_buffers(
    MirSurface *surface,
    mir_surface_callback callback,
    void *context);

/**
 * Advance a surface's buffer as in mir_surface_swap_buffers(), but also wait
 * for the operation to complete.
 *   \param [in] surface  The surface whose buffer to advance
 */
void mir_surface_swap_buffers_sync(MirSurface *surface);

/**
 * Release the supplied surface and any associated buffer. The returned wait
 * handle remains valid until the connection to the server is released.
 *   \warning callback could be called from another thread. You must do any
 *            locking appropriate to protect your data accessed in the
 *            callback.
 *   \param [in] surface      The surface
 *   \param [in] callback     Callback function to be invoked when the request
 *                            completes
 *   \param [in,out] context  User data passed to the callback function
 *   \return                  A handle that can be passed to mir_wait_for
 */
MirWaitHandle *mir_surface_release(
    MirSurface *surface,
    mir_surface_callback callback,
    void *context);

/**
 * Release the specified surface like in mir_surface_release(), but also wait
 * for the operation to complete.
 *   \param [in] surface  The surface to be released
 */
void mir_surface_release_sync(MirSurface *surface);

/**
 * Wait on the supplied handle until all instances of the associated request
 * have completed.
 *   \param [in] wait_handle  Handle returned by an asynchronous request
 */
void mir_wait_for(MirWaitHandle *wait_handle);

/**
 * Wait on the supplied handle until one instance of the associated request
 * has completed. Use this instead of mir_wait_for in a threaded environment
 * to ensure that the act of waiting does not clear all results associated
 * with the wait handle; only one.
 *   \param [in] wait_handle  Handle returned by an asynchronous request
 */
void mir_wait_for_one(MirWaitHandle *wait_handle);

/**
 * \deprecated Use mir_debug_surface_id()
 */
__attribute__((__deprecated__("Use mir_debug_surface_id()")))
int mir_surface_get_id(MirSurface *surface);

/**
 * Set the type (purpose) of a surface. This is not guaranteed to always work
 * with some shell types (e.g. phone/tablet UIs). As such, you may have to
 * wait on the function and check the result using mir_surface_get_type.
 *   \param [in] surface  The surface to operate on
 *   \param [in] type     The new type of the surface
 *   \return              A wait handle that can be passed to mir_wait_for
 */
MirWaitHandle* mir_surface_set_type(MirSurface *surface, MirSurfaceType type);

/**
 * Get the type (purpose) of a surface.
 *   \param [in] surface  The surface to query
 *   \return              The type of the surface
 */
MirSurfaceType mir_surface_get_type(MirSurface *surface);

/**
 * Change the state of a surface.
 *   \param [in] surface  The surface to operate on
 *   \param [in] state    The new state of the surface
 *   \return              A wait handle that can be passed to mir_wait_for
 */
MirWaitHandle* mir_surface_set_state(MirSurface *surface,
                                     MirSurfaceState state);

/**
 * Get the current state of a surface.
 *   \param [in] surface  The surface to query
 *   \return              The state of the surface
 */
MirSurfaceState mir_surface_get_state(MirSurface *surface);

/**
 * Set the swapinterval for mir_surface_swap_buffers. EGL users should use
 * eglSwapInterval directly.
 * At the time being, only swapinterval of 0 or 1 is supported. 
 *   \param [in] surface  The surface to operate on
 *   \param [in] interval The number of vblank signals that
 *                        mir_surface_swap_buffers will wait for 
 *   \return              A wait handle that can be passed to mir_wait_for,
 *                        or NULL if the interval could not be supported 
 */
MirWaitHandle* mir_surface_set_swapinterval(MirSurface* surface, int interval);

/**
 * Query the swapinterval that the surface is operating with.
 * The default interval is 1.
 *   \param [in] surface  The surface to operate on
 *   \return              The swapinterval value that the client is operating with.
 *                        Returns -1 if surface is invalid.
 */
int mir_surface_get_swapinterval(MirSurface* surface);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_CLIENT_LIBRARY_H */
