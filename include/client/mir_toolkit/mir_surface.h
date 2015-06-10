/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_MIR_SURFACE_H_
#define MIR_TOOLKIT_MIR_SURFACE_H_

#include <mir_toolkit/mir_native_buffer.h>
#include <mir_toolkit/client_types.h>
#include <mir_toolkit/common.h>
#include <mir_toolkit/mir_cursor_configuration.h>

#include <stdbool.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Create a surface specification for a normal surface.
 *
 * A normal surface is suitable for most application windows. It has no special semantics.
 * On a desktop shell it will typically have a title-bar, be movable, resizeable, etc.
 *
 * \param [in] connection   Connection the surface will be created on
 * \param [in] width        Requested width. The server is not guaranteed to return a surface of this width.
 * \param [in] height       Requested height. The server is not guaranteed to return a surface of this height.
 * \param [in] format       Pixel format for the surface.
 * \return                  A handle that can be passed to mir_surface_create() to complete construction.
 */
MirSurfaceSpec* mir_connection_create_spec_for_normal_surface(MirConnection* connection,
                                                              int width,
                                                              int height,
                                                              MirPixelFormat format);

/**
 * Create a surface specification for a menu surface.
 *
 * Positioning of the surface is specified with respect to the parent surface
 * via an adjacency rectangle. The server will attempt to choose an edge of the
 * adjacency rectangle on which to place the surface taking in to account
 * screen-edge proximity or similar constraints. In addition, the server can use
 * the edge affinity hint to consider only horizontal or only vertical adjacency
 * edges in the given rectangle.
 *
 * \param [in] connection   Connection the surface will be created on
 * \param [in] width        Requested width. The server is not guaranteed to
 *                          return a surface of this width.
 * \param [in] height       Requested height. The server is not guaranteed to
 *                          return a surface of this height.
 * \param [in] format       Pixel format for the surface.
 * \param [in] parent       A valid parent surface for this menu.
 * \param [in] rect         The adjacency rectangle. The server is not
 *                          guaranteed to create a surface at the requested
 *                          location.
 * \param [in] edge         The preferred edge direction to attach to. Use
 *                          mir_edge_attachment_any for no preference.
 * \return                  A handle that can be passed to mir_surface_create()
 *                          to complete construction.
 */
MirSurfaceSpec*
mir_connection_create_spec_for_menu(MirConnection* connection,
                                    int width,
                                    int height,
                                    MirPixelFormat format,
                                    MirSurface* parent,
                                    MirRectangle* rect,
                                    MirEdgeAttachment edge);

/**
 * Create a surface specification for a tooltip surface.
 *
 * A tooltip surface becomes visible when the pointer hovers the specified
 * target zone. A tooltip surface has no input focus and will be closed when
 * the pointer moves out of the target zone or the parent closes, moves or hides
 *
 * The tooltip parent cannot be another tooltip surface.
 *
 * The tooltip position is decided by the server but typically it will appear
 * near the pointer.
 *
 * \param [in] connection   Connection the surface will be created on
 * \param [in] width        Requested width. The server is not guaranteed to
 *                          return a surface of this width.
 * \param [in] height       Requested height. The server is not guaranteed to
 *                          return a surface of this height.
 * \param [in] format       Pixel format for the surface.
 * \param [in] parent       A valid parent surface for this tooltip.
 * \param [in] rect         A target zone relative to parent.
 * \return                  A handle that can be passed to mir_surface_create()
 *                          to complete construction.
 */
MirSurfaceSpec*
mir_connection_create_spec_for_tooltip(MirConnection* connection,
                                       int width,
                                       int height,
                                       MirPixelFormat format,
                                       MirSurface* parent,
                                       MirRectangle* zone);

/**
 * Create a surface specification for a modal dialog surface.
 *
 * The dialog surface will have input focus; the parent can still be moved,
 * resized or hidden/minimized but no interaction is possible until the dialog
 * is dismissed.
 *
 * A dialog will typically have no close/maximize button decorations.
 *
 * During surface creation, if the specified parent is another dialog surface
 * the server may choose to close the specified parent in order to show this
 * new dialog surface.
 *
 * \param [in] connection   Connection the surface will be created on
 * \param [in] width        Requested width. The server is not guaranteed to
 *                          return a surface of this width.
 * \param [in] height       Requested height. The server is not guaranteed to
 *                          return a surface of this height.
 * \param [in] format       Pixel format for the surface.
 * \param [in] parent       A valid parent surface.
 *
 */
MirSurfaceSpec*
mir_connection_create_spec_for_modal_dialog(MirConnection* connection,
                                            int width,
                                            int height,
                                            MirPixelFormat format,
                                            MirSurface* parent);

/**
 * Create a surface specification for a parentless dialog surface.
 *
 * A parentless dialog surface is similar to a normal surface, but it cannot
 * be fullscreen and typically won't have any maximize/close button decorations.
 *
 * A parentless dialog is not allowed to have other dialog children. The server
 * may decide to close the parent and show the child dialog only.
 *
 * \param [in] connection   Connection the surface will be created on
 * \param [in] width        Requested width. The server is not guaranteed to
 *                          return a surface of this width.
 * \param [in] height       Requested height. The server is not guaranteed to
 *                          return a surface of this height.
 * \param [in] format       Pixel format for the surface.
 *
 */
MirSurfaceSpec*
mir_connection_create_spec_for_dialog(MirConnection* connection,
                                      int width,
                                      int height,
                                      MirPixelFormat format);

/**
 * Create a surface specification.
 * This can be used with mir_surface_create() to create a surface or with
 * mir_surface_apply_spec() to change an existing surface.
 * \remark For use with mir_surface_create() at least the type, width, height,
 * format and buffer_usage must be set. (And for types requiring a parent that
 * too must be set.)
 *
 * \param [in] connection   a valid mir connection
 * \return                  A handle that can ultimately be passed to
 *                          mir_surface_create() or mir_surface_apply_spec()
 */
MirSurfaceSpec* mir_create_surface_spec(MirConnection* connection);

/**
 * Create a surface specification for updating a surface.
 *
 * This can be applied to one or more target surfaces using
 * mir_surface_apply_spec(...).
 *
 * \param [in] connection   a valid mir connection
 *
 */
MirSurfaceSpec*
mir_connection_create_spec_for_changes(MirConnection* connection);

/**
 * Create a surface from a given specification
 *
 *
 * \param [in] requested_specification  Specification of the attributes for the created surface
 * \param [in] callback                 Callback function to be invoked when creation is complete
 * \param [in, out] context             User data passed to callback function.
 *                                      This callback is guaranteed to be called, and called with a
 *                                      non-null MirSurface*, but the surface may be invalid in
 *                                      case of an error.
 * \return                              A handle that can be passed to mir_wait_for()
 */
MirWaitHandle* mir_surface_create(MirSurfaceSpec* requested_specification,
                                  mir_surface_callback callback, void* context);

/**
 * Create a surface from a given specification and wait for the result.
 * \param [in] requested_specification  Specification of the attributes for the created surface
 * \return                              The new surface. This is guaranteed non-null, but may be invalid
 *                                      in the case of error.
 */
MirSurface* mir_surface_create_sync(MirSurfaceSpec* requested_specification);

/**
 * Set the requested parent.
 *
 * \param [in] spec    Specification to mutate
 * \param [in] parent  A valid parent surface.
 *
 * \return              true unless "parent" is invalid for this surface type.
 */
bool mir_surface_spec_set_parent(MirSurfaceSpec* spec, MirSurface* parent);

/**
 * Update a surface specification with a surface type.
 * This can be used with mir_surface_create() to create a surface or with
 * mir_surface_apply_spec() to change an existing surface.
 * \remark For use with mir_surface_apply_spec() the shell need not support
 * arbitrary changes of type and some target types may require the setting of
 * properties such as "parent" that are not already present on the surface.
 * The type transformations the server is required to support are:\n
 * regular => utility, dialog or satellite\n
 * utility => regular, dialog or satellite\n
 * dialog => regular, utility or satellite\n
 * satellite => regular, utility or dialog\n
 * popup => satellite
 *
 * \param [in] spec         Specification to mutate
 * \param [in] type         the target type of the surface
 *
 * \return                  true (pointless consistency)
 */
bool mir_surface_spec_set_type(MirSurfaceSpec* spec, MirSurfaceType type);

/**
 * Set the requested name.
 *
 * The surface name helps the user to distinguish between multiple surfaces
 * from the same application. A typical desktop shell may use it to provide
 * text in the window titlebar, in an alt-tab switcher, or equivalent.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] name     Requested name. This must be valid UTF-8.
 *                      Copied into spec; clients can free the buffer passed after this call.
 * \return              False if name is not a valid attribute of this surface type.
 */
bool mir_surface_spec_set_name(MirSurfaceSpec* spec, char const* name);

/**
 * Set the requested width, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    Requested width.
 * \return              False if width is invalid for a surface of this type
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          surface of any specific width or height.
 */
bool mir_surface_spec_set_width(MirSurfaceSpec* spec, unsigned width);

/**
 * Set the requested height, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] height   Requested height.
 * \return              False if height is invalid for a surface of this type
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          surface of any specific width or height.
 */
bool mir_surface_spec_set_height(MirSurfaceSpec* spec, unsigned height);

/**
 * Set the requested width increment, in pixels.
 * Defines an arithmetic progression of sizes starting with min_width (if set, otherwise 0)
 * into which the surface prefers to be resized.
 *
 * \param [in] spec       Specification to mutate
 * \param [in] width_inc  Requested width increment.
 * \return                False if width increment is invalid for a surface of this type
 * \note    The requested dimensions are a hint only. The server is not guaranteed to
 *          create a surface of any specific width or height.
 */
bool mir_surface_spec_set_width_increment(MirSurfaceSpec *spec, unsigned width_inc);

/**
 * Set the requested height increment, in pixels
 * Defines an arithmetic progression of sizes starting with min_height (if set, otherwise 0)
 * into which the surface prefers to be resized.
 *
 * \param [in] spec       Specification to mutate
 * \param [in] height_inc Requested height increment.
 * \return                False if height increment is invalid for a surface of this type
 * \note    The requested dimensions are a hint only. The server is not guaranteed to
 *          create a surface of any specific width or height.
 */
bool mir_surface_spec_set_height_increment(MirSurfaceSpec *spec, unsigned height_inc);

/**
 * Set the minimum width, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    Minimum width.
 * \return              False if minimum width is invalid for a surface of this type
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          surface of any specific width or height.
 */
bool mir_surface_spec_set_min_width(MirSurfaceSpec* spec, unsigned min_width);

/**
 * Set the minimum height, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] height   Minimum height.
 * \return              False if minimum height is invalid for a surface of this type
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          surface of any specific width or height.
 */
bool mir_surface_spec_set_min_height(MirSurfaceSpec* spec, unsigned min_height);
/**
 * Set the maximum width, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    Maximum width.
 * \return              False if maximum width is invalid for a surface of this type
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          surface of any specific width or height.
 */
bool mir_surface_spec_set_max_width(MirSurfaceSpec* spec, unsigned max_width);

/**
 * Set the maximum height, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] height   Maximum height.
 * \return              False if maximum height is invalid for a surface of this type
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          surface of any specific width or height.
 */
bool mir_surface_spec_set_max_height(MirSurfaceSpec* spec, unsigned max_height);

/**
 * Set the minimum aspect ratio. This is the minimum ratio of surface width to height.
 * It is independent of orientation changes and/or preferences.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    numerator
 * \param [in] height   denominator
 * \return              False if minimum aspect is invalid for a surface of this type
 * \note    The requested aspect ratio is a hint only. The server is not guaranteed
 *          to create a surface of any specific aspect.
 */
bool mir_surface_spec_set_min_aspect_ratio(MirSurfaceSpec* spec, unsigned width, unsigned height);

/**
 * Set the maximum aspect ratio. This is the maximum ratio of surface width to height.
 * It is independent of orientation changes and/or preferences.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    numerator
 * \param [in] height   denominator
 * \return              False if maximum aspect is invalid for a surface of this type
 * \note    The requested aspect ratio is a hint only. The server is not guaranteed
 *          to create a surface of any specific aspect.
 */
bool mir_surface_spec_set_max_aspect_ratio(MirSurfaceSpec* spec, unsigned width, unsigned height);

/**
 * Set the requested pixel format.
 * \param [in] spec     Specification to mutate
 * \param [in] format   Requested pixel format
 * \return              False if format is not a valid pixel format for this surface type.
 * \note    If this call returns %true then the server is guaranteed to honour this request.
 *          If the server is unable to create a surface with this pixel format at
 *          the point mir_surface_create() is called it will instead return an invalid surface.
 */
bool mir_surface_spec_set_pixel_format(MirSurfaceSpec* spec, MirPixelFormat format);

/**
 * Set the requested buffer usage.
 * \param [in] spec     Specification to mutate
 * \param [in] usage    Requested buffer usage
 * \return              False if the requested buffer usage is invalid for this surface.
 * \note    If this call returns %true then the server is guaranteed to honour this request.
 *          If the server is unable to create a surface with this buffer usage at
 *          the point mir_surface_create() is called it will instead return an invalid surface.
 */
bool mir_surface_spec_set_buffer_usage(MirSurfaceSpec* spec, MirBufferUsage usage);

/**
 * \param [in] spec         Specification to mutate
 * \param [in] output_id    ID of output to place surface on. From MirDisplayOutput.output_id
 * \return                  False if setting surface fullscreen is invalid for this surface.
 * \note    If this call returns %true then a valid surface returned from mir_surface_create() is
 *          guaranteed to be fullscreen on the specified output. An invalid surface is returned
 *          if the server unable to, or policy prevents it from, honouring this request.
 */
bool mir_surface_spec_set_fullscreen_on_output(MirSurfaceSpec* spec, uint32_t output_id);

/**
 * Set the requested preferred orientation mode.
 * \param [in] spec    Specification to mutate
 * \param [in] mode    Requested preferred orientation
 * \return             False if the mode is not valid for this surface type.
 * \note    If the server is unable to create a surface with the preferred orientation at
 *          the point mir_surface_create() is called it will instead return an invalid surface.
 */
bool mir_surface_spec_set_preferred_orientation(MirSurfaceSpec* spec, MirOrientationMode mode);

/**
 * Set the requested state.
 * \param [in] spec    Specification to mutate
 * \param [in] mode    Requested state
 * \return             False if the state is not valid for this surface type.
 * \note    If the server is unable to create a surface with the requested state at
 *          the point mir_surface_create() is called it will instead return an invalid surface.
 */
bool mir_surface_spec_set_state(MirSurfaceSpec* spec, MirSurfaceState state);

/**
 * Release the resources held by a MirSurfaceSpec.
 *
 * \param [in] spec     Specification to release
 */
void mir_surface_spec_release(MirSurfaceSpec* spec);

/**
 * Set the event handler to be called when events arrive for a surface.
 *   \warning event_handler could be called from another thread. You must do
 *            any locking appropriate to protect your data accessed in the
 *            callback. There is also a chance that different events will be
 *            called back in different threads, for the same surface,
 *            simultaneously.
 *   \param [in] surface        The surface
 *   \param [in] callback       The callback function
 *   \param [in] context        Additional argument to be passed to callback
 */
void mir_surface_set_event_handler(MirSurface *surface,
                                   mir_surface_event_callback callback,
                                   void* context);

/**
 * Retrieve the primary MirBufferStream associated with a surface (to advance buffers,
 * obtain EGLNativeWindow, etc...)
 * 
 *   \param[in] surface The surface
 */
MirBufferStream* mir_surface_get_buffer_stream(MirSurface *surface);

/**
 * Test for a valid surface
 *   \param [in] surface  The surface
 *   \return              True if the supplied surface is valid, or
 *                        false otherwise.
 */
bool mir_surface_is_valid(MirSurface *surface);

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

/**
 * Query the DPI value of the surface (dots per inch). This will vary depending
 * on the physical display configuration and where the surface is within it.
 *   \return  The DPI of the surface, or zero if unknown.
 */
int mir_surface_get_dpi(MirSurface* surface);
    
/**
 * Query the focus state for a surface.
 *   \param [in] surface The surface to operate on
 *   \return             The focus state of said surface
 */
MirSurfaceFocusState mir_surface_get_focus(MirSurface *surface);

/**
 * Query the visibility state for a surface.
 *   \param [in] surface The surface to operate on
 *   \return             The visibility state of said surface
 */
MirSurfaceVisibility mir_surface_get_visibility(MirSurface *surface);

/**
 * Choose the cursor state for a surface: whether a cursor is shown, 
 * and which cursor if so.
 *    \param [in] surface    The surface to operate on
 *    \param [in] parameters The configuration parameters obtained
 *                           from mir_cursor* family of functions.
 *    \return                A wait handle that can be passed to mir_wait_for,
 *                           or NULL if parameters is invalid.
 *
 */
MirWaitHandle* mir_surface_configure_cursor(MirSurface *surface, MirCursorConfiguration const* parameters);

/**
 * Get the orientation of a surface.
 *   \param [in] surface  The surface to query
 *   \return              The orientation of the surface
 */
MirOrientation mir_surface_get_orientation(MirSurface *surface);

/**
 * Request to set the preferred orientations of a surface.
 * The request may be rejected by the server; to check wait on the
 * result and check the applied value using mir_surface_get_preferred_orientation
 *   \param [in] surface     The surface to operate on
 *   \param [in] orientation The preferred orientation modes
 *   \return                 A wait handle that can be passed to mir_wait_for
 */
MirWaitHandle* mir_surface_set_preferred_orientation(MirSurface *surface, MirOrientationMode orientation);

/**
 * Get the preferred orientation modes of a surface.
 *   \param [in] surface  The surface to query
 *   \return              The preferred orientation modes
 */
MirOrientationMode mir_surface_get_preferred_orientation(MirSurface *surface);

/**
 * Create a surface specification for an input method surface.
 *
 * Currently this is only appropriate for the Unity On-Screen-Keyboard.
 *
 * \param [in] connection   Connection the surface will be created on
 * \param [in] width        Requested width. The server is not guaranteed to return a surface of this width.
 * \param [in] height       Requested height. The server is not guaranteed to return a surface of this height.
 * \param [in] format       Pixel format for the surface.
 * \return                  A handle that can be passed to mir_surface_create() to complete construction.
 */
MirSurfaceSpec* mir_connection_create_spec_for_input_method(MirConnection* connection,
                                                            int width,
                                                            int height,
                                                            MirPixelFormat format);

/**
 * Request changes to the specification of a surface. The server will decide
 * whether and how the request can be honoured.
 *
 *   \param [in] surface  The surface to rename
 *   \param [in] spec     Spec with the requested changes applied
 */
void mir_surface_apply_spec(MirSurface* surface, MirSurfaceSpec* spec);

/**
 * \brief Request an ID for the surface that can be shared cross-process and
 *        across restarts.
 *
 * This call acquires a MirPersistentId for this MirSurface. This MirPersistentId
 * can be serialized to a string, stored or sent to another process, and then
 * later deserialized to refer to the same surface.
 *
 * \param [in]     surface   The surface to acquire a persistent reference to.
 * \param [in]     callback  Callback to invoke when the request completes.
 * \param [in,out] context   User data passed to completion callback.
 * \return A MirWaitHandle that can be used in mir_wait_for to await completion.
 */
MirWaitHandle* mir_surface_request_persistent_id(MirSurface* surface, mir_surface_id_callback callback, void* context);

/**
 * \brief Request a persistent ID for a surface and wait for the result
 * \param [in] surface  The surface to acquire a persistent ID for.
 * \return A MirPersistentId. This MirPersistentId is owned by the calling code, and must
 *         be freed with a call to mir_persistent_id_release()
 */
MirPersistentId* mir_surface_request_persistent_id_sync(MirSurface *surface);

/**
 * \brief Check the validity of a MirPersistentId
 * \param [in] id  The MirPersistentId
 * \return True iff the MirPersistentId contains a valid ID value.
 *
 * \note This does not guarantee that the ID refers to a currently valid object.
 */
bool mir_persistent_id_is_valid(MirPersistentId* id);

/**
 * \brief Free a MirPersistentId
 * \param [in] id  The MirPersistentId to free
 * \note This frees only the client-side representation; it has no effect on the
 *       object referred to by \arg id.
 */
void mir_persistent_id_release(MirPersistentId* id);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_SURFACE_H_ */
