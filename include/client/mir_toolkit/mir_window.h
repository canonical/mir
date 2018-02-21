/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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

#ifndef MIR_TOOLKIT_MIR_WINDOW_H_
#define MIR_TOOLKIT_MIR_WINDOW_H_

#include <mir_toolkit/mir_native_buffer.h>
#include <mir_toolkit/client_types.h>
#include <mir_toolkit/common.h>
#include <mir_toolkit/mir_cursor_configuration.h>
#include <mir_toolkit/deprecations.h>

#include <stdbool.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Create a window specification for a normal window.
 *
 * A normal window is suitable for most application windows. It has no special semantics.
 * On a desktop shell it will typically have a title-bar, be movable, resizeable, etc.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to return a window of this height.
 *
 * \return                  A handle that can be passed to mir_create_window() to complete construction.
 */
MirWindowSpec* mir_create_normal_window_spec(MirConnection* connection,
                                             int width, int height);

/**
 * Create a window specification for a menu window.
 *
 * Positioning of the window is specified with respect to the parent window
 * via an adjacency rectangle. The server will attempt to choose an edge of the
 * adjacency rectangle on which to place the window taking in to account
 * screen-edge proximity or similar constraints. In addition, the server can use
 * the edge affinity hint to consider only horizontal or only vertical adjacency
 * edges in the given rectangle.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to
 *                          return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to
 *                          return a window of this height.
 * \param [in] parent       A valid parent window for this menu.
 * \param [in] rect         The adjacency rectangle. The server is not
 *                          guaranteed to create a window at the requested
 *                          location.
 * \param [in] edge         The preferred edge direction to attach to. Use
 *                          mir_edge_attachment_any for no preference.
 * \return                  A handle that can be passed to mir_create_window()
 *                          to complete construction.
 */
MirWindowSpec*
mir_create_menu_window_spec(MirConnection* connection,
                            int width, int height,
                            MirWindow* parent,
                            MirRectangle* rect,
                            MirEdgeAttachment edge);

/**
 * Create a window specification for a tip window.
 *
 * Positioning of the window is specified with respect to the parent window
 * via an adjacency rectangle. The server will attempt to choose an edge of the
 * adjacency rectangle on which to place the window taking in to account
 * screen-edge proximity or similar constraints. In addition, the server can use
 * the edge affinity hint to consider only horizontal or only vertical adjacency
 * edges in the given rectangle.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to
 *                          return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to
 *                          return a window of this height.
 * \param [in] parent       A valid parent window for this tip.
 * \param [in] rect         The adjacency rectangle. The server is not
 *                          guaranteed to create a window at the requested
 *                          location.
 * \param [in] edge         The preferred edge direction to attach to. Use
 *                          mir_edge_attachment_any for no preference.
 * \return                  A handle that can be passed to mir_create_window()
 *                          to complete construction.
 */
MirWindowSpec*
mir_create_tip_window_spec(MirConnection* connection,
                           int width, int height,
                           MirWindow* parent,
                           MirRectangle* rect,
                           MirEdgeAttachment edge);

/**
 * Create a window specification for a modal dialog window.
 *
 * The dialog window will have input focus; the parent can still be moved,
 * resized or hidden/minimized but no interaction is possible until the dialog
 * is dismissed.
 *
 * A dialog will typically have no close/maximize button decorations.
 *
 * During window creation, if the specified parent is another dialog window
 * the server may choose to close the specified parent in order to show this
 * new dialog window.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to
 *                          return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to
 *                          return a window of this height.
 * \param [in] parent       A valid parent window.
 *
 */
MirWindowSpec*
mir_create_modal_dialog_window_spec(MirConnection* connection,
                                    int width, int height,
                                    MirWindow* parent);

/**
 * Create a window specification for a parentless dialog window.
 *
 * A parentless dialog window is similar to a normal window, but it cannot
 * be fullscreen and typically won't have any maximize/close button decorations.
 *
 * A parentless dialog is not allowed to have other dialog children. The server
 * may decide to close the parent and show the child dialog only.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to
 *                          return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to
 *                          return a window of this height.
 */
MirWindowSpec*
mir_create_dialog_window_spec(MirConnection* connection,
                              int width, int height);

/**
 * Create a window specification for an input method window.
 *
 * Currently this is only appropriate for the Unity On-Screen-Keyboard.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to return a window of this height.
 * \return                  A handle that can be passed to mir_create_window() to complete construction.
 */
MirWindowSpec*
mir_create_input_method_window_spec(MirConnection* connection,
                                    int width, int height);

/**
 * Create a window specification for a gloss window.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to return a window of this height.
 * \return                  A handle that can be passed to mir_create_window() to complete construction.
 */
MirWindowSpec*
mir_create_gloss_window_spec(MirConnection* connection, int width, int height);

/**
 * Create a window specification for a satellite window.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to return a window of this height.
 * \param [in] parent       A valid parent window.
 * \return                  A handle that can be passed to mir_create_window() to complete construction.
 */
MirWindowSpec*
mir_create_satellite_window_spec(MirConnection* connection, int width, int height, MirWindow* parent);

/**
 * Create a window specification for a utility window.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to return a window of this height.
 * \return                  A handle that can be passed to mir_create_window() to complete construction.
 */
MirWindowSpec*
mir_create_utility_window_spec(MirConnection* connection, int width, int height);

/**
 * Create a window specification for a freestyle window.
 *
 * \param [in] connection   Connection the window will be created on
 * \param [in] width        Requested width. The server is not guaranteed to return a window of this width.
 * \param [in] height       Requested height. The server is not guaranteed to return a window of this height.
 * \return                  A handle that can be passed to mir_create_window() to complete construction.
 */
MirWindowSpec*
mir_create_freestyle_window_spec(MirConnection* connection, int width, int height);

/**
 * Create a window specification.
 * This can be used with mir_create_window() to create a window or with
 * mir_window_apply_spec() to change an existing window.
 * \remark For use with mir_create_window() at least the type, width and height
 * must be set. (And for types requiring a parent that too must be set.)
 *
 * \param [in] connection   a valid mir connection
 * \return                  A handle that can ultimately be passed to
 *                          mir_create_window() or mir_window_apply_spec()
 */
MirWindowSpec* mir_create_window_spec(MirConnection* connection);

/**
 * Retrieve the connection.
 * \remark this is the same connection as used to create the Window and does
 * not need an additional mir_connection_release().
 *
 * \param [in] window       a valid MirWindow
 * \return                  the connection
 */
MirConnection* mir_window_get_connection(MirWindow* window);

/**
 * Set the requested parent.
 *
 * \param [in] spec    Specification to mutate
 * \param [in] parent  A valid parent window.
 */
void mir_window_spec_set_parent(MirWindowSpec* spec, MirWindow* parent);

/**
 * Update a window specification with a window type.
 * This can be used with mir_create_window() to create a window or with
 * mir_window_apply_spec() to change an existing window.
 * \remark For use with mir_window_apply_spec() the shell need not support
 * arbitrary changes of type and some target types may require the setting of
 * properties such as "parent" that are not already present on the window.
 * The type transformations the server is required to support are:\n
 * regular => utility, dialog or satellite\n
 * utility => regular, dialog or satellite\n
 * dialog => regular, utility or satellite\n
 * satellite => regular, utility or dialog\n
 * popup => satellite
 *
 * \param [in] spec         Specification to mutate
 * \param [in] type         the target type of the window
 */
void mir_window_spec_set_type(MirWindowSpec* spec, MirWindowType type);

/**
 * Set the requested name.
 *
 * The window name helps the user to distinguish between multiple surfaces
 * from the same application. A typical desktop shell may use it to provide
 * text in the window titlebar, in an alt-tab switcher, or equivalent.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] name     Requested name. This must be valid UTF-8.
 *                      Copied into spec; clients can free the buffer passed after this call.
 */
void mir_window_spec_set_name(MirWindowSpec* spec, char const* name);

/**
 * Set the requested width, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    Requested width.
 *
 * \note    The requested dimensions are a hint only. The server is not
 *          guaranteed to create a window of any specific width or height.
 */
void mir_window_spec_set_width(MirWindowSpec* spec, unsigned width);

/**
 * Set the requested height, in pixels
 *
 * \param [in] spec     Specification to mutate
 * \param [in] height   Requested height.
 *
 * \note    The requested dimensions are a hint only. The server is not
 *          guaranteed to create a window of any specific width or height.
 */
void mir_window_spec_set_height(MirWindowSpec* spec, unsigned height);

/**
 * Set the requested width increment, in pixels.
 * Defines an arithmetic progression of sizes starting with min_width (if set, otherwise 0)
 * into which the window prefers to be resized.
 *
 * \param [in] spec       Specification to mutate
 * \param [in] width_inc  Requested width increment.
 *
 * \note    The requested dimensions are a hint only. The server is not guaranteed to
 *          create a window of any specific width or height.
 */
void mir_window_spec_set_width_increment(MirWindowSpec* spec, unsigned width_inc);

/**
 * Set the requested height increment, in pixels
 * Defines an arithmetic progression of sizes starting with min_height (if set, otherwise 0)
 * into which the window prefers to be resized.
 *
 * \param [in] spec       Specification to mutate
 * \param [in] height_inc Requested height increment.
 *
 * \note    The requested dimensions are a hint only. The server is not guaranteed to
 *          create a window of any specific width or height.
 */
void mir_window_spec_set_height_increment(MirWindowSpec* spec, unsigned height_inc);

/**
 * Set the minimum width, in pixels
 *
 * \param [in] spec       Specification to mutate
 * \param [in] min_width  Minimum width.
 *
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          window of any specific width or height.
 */
void mir_window_spec_set_min_width(MirWindowSpec* spec, unsigned min_width);

/**
 * Set the minimum height, in pixels
 *
 * \param [in] spec       Specification to mutate
 * \param [in] min_height Minimum height.
 *
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          window of any specific width or height.
 */
void mir_window_spec_set_min_height(MirWindowSpec* spec, unsigned min_height);

/**
 * Set the maximum width, in pixels
 *
 * \param [in] spec       Specification to mutate
 * \param [in] max_width  maximum width.
 *
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          window of any specific width or height.
 */
void mir_window_spec_set_max_width(MirWindowSpec* spec, unsigned max_width);

/**
 * Set the maximum height, in pixels
 *
 * \param [in] spec       Specification to mutate
 * \param [in] max_height maximum height.
 *
 * \note    The requested dimensions are a hint only. The server is not guaranteed to create a
 *          window of any specific width or height.
 */
void mir_window_spec_set_max_height(MirWindowSpec* spec, unsigned max_height);

/**
 * Set the minimum aspect ratio. This is the minimum ratio of window width to height.
 * It is independent of orientation changes and/or preferences.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    numerator
 * \param [in] height   denominator
 *
 * \note    The requested aspect ratio is a hint only. The server is not guaranteed
 *          to create a window of any specific aspect.
 */
void mir_window_spec_set_min_aspect_ratio(MirWindowSpec* spec, unsigned width, unsigned height);

/**
 * Set the maximum aspect ratio. This is the maximum ratio of window width to height.
 * It is independent of orientation changes and/or preferences.
 *
 * \param [in] spec     Specification to mutate
 * \param [in] width    numerator
 * \param [in] height   denominator
 *
 * \note    The requested aspect ratio is a hint only. The server is not guaranteed
 *          to create a window of any specific aspect.
 */
void mir_window_spec_set_max_aspect_ratio(MirWindowSpec* spec, unsigned width, unsigned height);

/**
 * \param [in] spec         Specification to mutate
 * \param [in] output_id    ID of output to place window on. From MirDisplayOutput.output_id
 *
 * \note    If this call returns %true then a valid window returned from mir_create_window() is
 *          guaranteed to be fullscreen on the specified output. An invalid window is returned
 *          if the server unable to, or policy prevents it from, honouring this request.
 */
void mir_window_spec_set_fullscreen_on_output(MirWindowSpec* spec, uint32_t output_id);

/**
 * Set the requested preferred orientation mode.
 * \param [in] spec    Specification to mutate
 * \param [in] mode    Requested preferred orientation
 *
 * \note    If the server is unable to create a window with the preferred orientation at
 *          the point mir_create_window() is called it will instead return an invalid window.
 */
void mir_window_spec_set_preferred_orientation(MirWindowSpec* spec, MirOrientationMode mode);

/**
 * Request that the created window be attached to a window of a different client.
 *
 * This is restricted to input methods, which need to attach their suggestion window
 * to text entry widgets of other processes.
 *
 * \param [in] spec             Specification to mutate
 * \param [in] parent           A MirWindowId reference to the parent window
 * \param [in] attachment_rect  A rectangle specifying the region (in parent window coordinates)
 *                              that the created window should be attached to.
 * \param [in] edge             The preferred edge direction to attach to. Use
 *                              mir_edge_attachment_any for no preference.
 * \return                      False if the operation is invalid for this window type.
 *
 * \note    If the parent window becomes invalid before mir_create_window() is processed,
 *          it will return an invalid window. If the parent window is valid at the time
 *          mir_create_window() is called but is later closed, this window will also receive
 *          a close event.
 */
bool mir_window_spec_attach_to_foreign_parent(MirWindowSpec* spec,
                                              MirWindowId* parent,
                                              MirRectangle* attachment_rect,
                                              MirEdgeAttachment edge);

/**
 * Set the requested state.
 * \param [in] spec    Specification to mutate
 * \param [in] state   Requested state
 *
 * \note    If the server is unable to create a window with the requested state at
 *          the point mir_create_window() is called it will instead return an invalid window.
 */
void mir_window_spec_set_state(MirWindowSpec* spec, MirWindowState state);

/**
 * Set a collection of input rectangles associated with the spec.
 * Rectangles are specified as a list of regions relative to the top left
 * of the specified window. If the server applies this specification
 * to a window input which would normally go to the window but is not
 * contained within any of the input rectangles instead passes
 * on to the next client.
 *
 * \param [in] spec The spec to accumulate the request in.
 * \param [in] rectangles An array of MirRectangles specifying the input shape.
 * \param [in] n_rects The number of elements in the rectangles array.
 */
void mir_window_spec_set_input_shape(MirWindowSpec* spec,
                                     MirRectangle const *rectangles,
                                     size_t n_rects);

/**
 * Set the event handler to be called when events arrive for a window.
 *   \warning event_handler could be called from another thread. You must do
 *            any locking appropriate to protect your data accessed in the
 *            callback. There is also a chance that different events will be
 *            called back in different threads, for the same window,
 *            simultaneously.
 * \param [in] spec       The spec to accumulate the request in.
 * \param [in] callback   The callback function
 * \param [in] context    Additional argument to be passed to callback
 */
void mir_window_spec_set_event_handler(MirWindowSpec* spec,
                                       MirWindowEventCallback callback,
                                       void* context);

/**
 * Ask the shell to customize "chrome" for this window.
 * For example, on the phone hide indicators when this window is active.
 *
 * \param [in] spec The spec to accumulate the request in.
 * \param [in] style The requested level of "chrome"
 */
void mir_window_spec_set_shell_chrome(MirWindowSpec* spec, MirShellChrome style);

/**
 * Attempts to set the pointer confinement spec for this window
 *
 * This will request the window manager to confine the pointer to the surfaces region.
 *
 * \param [in] spec  The spec to accumulate the request in.
 * \param [in] state The state you would like the pointer confinement to be in.
 */
void mir_window_spec_set_pointer_confinement(MirWindowSpec* spec, MirPointerConfinementState state);

/**
 * Set the window placement on the spec.
 *
 * \param [in] spec             the spec to update
 * \param [in] rect             the destination rectangle to align with
 * \param [in] rect_gravity     the point on \p rect to align with
 * \param [in] window_gravity   the point on the window to align with
 * \param [in] placement_hints  positioning hints to use when limited on space
 * \param [in] offset_dx        horizontal offset to shift w.r.t. \p rect
 * \param [in] offset_dy        vertical offset to shift w.r.t. \p rect
 *
 * Moves a window to \p rect, aligning their reference points.
 *
 * \p rect is relative to the top-left corner of the parent window.
 * \p rect_gravity and \p surface_gravity determine the points on \p rect and
 * the window to pin together. \p rect's alignment point can be offset by
 * \p offset_dx and \p offset_dy, which is equivalent to offsetting the
 * position of the window.
 *
 * \p placement_hints determine how the window should be positioned in the case
 * that the window would fall off-screen if placed in its ideal position.
 * See \ref MirPlacementHints for details.
 */
void mir_window_spec_set_placement(MirWindowSpec*      spec,
                                   const MirRectangle* rect,
                                   MirPlacementGravity rect_gravity,
                                   MirPlacementGravity window_gravity,
                                   MirPlacementHints   placement_hints,
                                   int                 offset_dx,
                                   int                 offset_dy);

/**
 * Set the name for the cursor from the system cursor theme.
 * \param [in] spec             The spec
 * \param [in] name             The name, or "" to reset to default
 */
void mir_window_spec_set_cursor_name(MirWindowSpec* spec, char const* name);

/**
 *
 * Set the requested pixel format.
 * \deprecated There will be no default stream associated with a window anymore. Instead create a
 *             MirRenderSurface and either set the pixel format through EGL (for EGL based rendering) or
 *             by allocating a cpu accessible buffer through mir_connection_allocate_buffer or
 *             mir_render_surface_get_buffer_stream
 * \param [in] spec     Specification to mutate
 * \param [in] format   Requested pixel format
 *
 * \note    If this call returns %true then the server is guaranteed to honour this request.
 *          If the server is unable to create a window with this pixel format at
 *          the point mir_create_window() is called it will instead return an invalid window.
 */
void mir_window_spec_set_pixel_format(MirWindowSpec* spec, MirPixelFormat format)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_connection_allocate_buffer/mir_render_surface_get_buffer_stream instead");

/**
 * \note To be deprecated soon. Only for enabling other deprecations.
 *
 * Set the requested buffer usage.
 * \deprecated There will be no default stream associated with a window anymore. MirBufferUsage is no longer applicable.
 * \param [in] spec     Specification to mutate
 * \param [in] usage    Requested buffer usage
 *
 * \note    If this call returns %true then the server is guaranteed to honour this request.
 *          If the server is unable to create a window with this buffer usage at
 *          the point mir_create_window() is called it will instead return an invalid window.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void mir_window_spec_set_buffer_usage(MirWindowSpec* spec, MirBufferUsage usage)
MIR_FOR_REMOVAL_IN_VERSION_1("No longer applicable, use mir_render_surface apis");
#pragma GCC diagnostic pop
/**
 *
 * Set the streams associated with the spec.
 * streams[0] is the bottom-most stream, and streams[size-1] is the topmost.
 * On application of the spec, a stream that is present in the window,
 * but is not in the list will be disassociated from the window.
 * On application of the spec, a stream that is not present in the window,
 * but is in the list will be associated with the window.
 * Streams set a displacement from the top-left corner of the window.
 *
 * \deprecated Use mir_window_spec_add_render_surface
 * \warning disassociating streams from the window will not release() them.
 * \warning It is wiser to arrange the streams within the bounds of the
 *          window the spec is applied to. Shells can define their own
 *          behavior as to what happens to an out-of-bound stream.
 *
 * \param [in] spec        The spec to accumulate the request in.
 * \param [in] streams     An array of non-null streams info.
 * \param [in] num_streams The number of elements in the streams array.
 */
void mir_window_spec_set_streams(MirWindowSpec* spec,
                                 MirBufferStreamInfo* streams,
                                 unsigned int num_streams)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_spec_add_render_surface instead");

/**
 * Release the resources held by a MirWindowSpec.
 *
 * \param [in] spec     Specification to release
 */
void mir_window_spec_release(MirWindowSpec* spec);

/**
 * Request changes to the specification of a window. The server will decide
 * whether and how the request can be honoured.
 *
 *   \param [in] window  The window to mutate
 *   \param [in] spec    Spec with the requested changes applied
 */
void mir_window_apply_spec(MirWindow* window, MirWindowSpec* spec);

/**
 * Create a window from a given specification
 *
 *
 * \param [in] requested_specification  Specification of the attributes for the created window
 * \param [in] callback                 Callback function to be invoked when creation is complete
 * \param [in, out] context             User data passed to callback function.
 *                                      This callback is guaranteed to be called, and called with a
 *                                      non-null MirWindow*, but the window may be invalid in
 *                                      case of an error.
 */
void mir_create_window(MirWindowSpec* requested_specification,
                       MirWindowCallback callback, void* context);

/**
 * Create a window from a given specification and wait for the result.
 * \param [in] requested_specification  Specification of the attributes for the created window
 * \return                              The new window. This is guaranteed non-null, but may be invalid
 *                                      in the case of error.
 */
MirWindow* mir_create_window_sync(MirWindowSpec* requested_specification);

/**
 * Release the supplied window and any associated buffer.
 *
 *   \warning callback could be called from another thread. You must do any
 *            locking appropriate to protect your data accessed in the
 *            callback.
 *   \param [in] window       The window
 *   \param [in] callback     Callback function to be invoked when the request
 *                            completes
 *   \param [in,out] context  User data passed to the callback function
 */
void mir_window_release(
    MirWindow* window,
    MirWindowCallback callback,
    void *context);

/**
 * Release the specified window like in mir_window_release(), but also wait
 * for the operation to complete.
 *   \param [in] window  The window to be released
 */
void mir_window_release_sync(MirWindow* window);

/**
 * Test for a valid window
 *   \param [in] window   The window
 *   \return              True if the supplied window is valid, or
 *                        false otherwise.
 */
bool mir_window_is_valid(MirWindow* window);

/**
 * Set the event handler to be called when events arrive for a window.
 *   \warning event_handler could be called from another thread. You must do
 *            any locking appropriate to protect your data accessed in the
 *            callback. There is also a chance that different events will be
 *            called back in different threads, for the same window,
 *            simultaneously.
 *   \param [in] window         The window
 *   \param [in] callback       The callback function
 *   \param [in] context        Additional argument to be passed to callback
 */
void mir_window_set_event_handler(MirWindow* window,
                                  MirWindowEventCallback callback,
                                  void* context);

/**
 * Retrieve the primary MirBufferStream associated with a window (to advance buffers,
 * obtain EGLNativeWindow, etc...)
 *
 *   \deprecated Users should use mir_window_spec_add_render_surface() to arrange
 *               the content of a window, instead of relying on a stream
 *               being created by default.
 *   \warning If the window was created with, or modified to have a
 *            MirWindowSpec containing streams added through
 *            mir_window_spec_set_streams(), the default stream will
 *            be removed, and this function will return NULL.
 *   \param[in] window The window
 */
MirBufferStream* mir_window_get_buffer_stream(MirWindow* window)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_spec_add_render_surface during window creation/modification instead");
/**
 * Retrieve a text description of the error. The returned string is owned by
 * the library and remains valid until the window or the associated
 * connection has been released.
 *   \param [in] window   The window
 *   \return              A text description of any error resulting in an
 *                        invalid window, or the empty string "" if the
 *                        connection is valid.
 */
char const* mir_window_get_error_message(MirWindow* window);

/**
 * Get a window's parameters.
 *  \deprecated Use mir_window getters or listen for state change events instead
 *  \pre                     The window is valid
 *  \param [in]  window      The window
 *  \param [out] parameters  Structure to be populated
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void mir_window_get_parameters(MirWindow* window, MirWindowParameters* parameters)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_get_xxx apis or listen to state/attribute change events instead");
#pragma GCC diagnostic pop


/**
 * Get the orientation of a window.
 *   \param [in] window  The window to query
 *   \return              The orientation of the window
 */
MirOrientation mir_window_get_orientation(MirWindow* window);

/**
 * Attempts to raise the window to the front.
 *
 * \param [in] window  The window to raise
 * \param [in] cookie  A cookie instance obtained from an input event.
 *                     An invalid cookie will terminate the client connection.
 */
void mir_window_raise(MirWindow* window, MirCookie const* cookie);

/**
 * Informs the window manager that the user is moving the window.
 *
 * \param [in] window  The window to move
 * \param [in] cookie  A cookie instance obtained from an input event.
 *                     An invalid cookie will terminate the client connection.
 */
void mir_window_request_user_move(MirWindow* window, MirCookie const* cookie);

/**
 * Informs the window manager that the user is resizing the window.
 *
 * \param [in] window  The window to resize
 * \param [in/ edge    The edge being dragged
 * \param [in] cookie  A cookie instance obtained from an input event.
 *                     An invalid cookie will terminate the client connection.
 */
void mir_window_request_user_resize(MirWindow* window, MirResizeEdge edge, MirCookie const* cookie);

/**
 * Get the type (purpose) of a window.
 *   \param [in] window   The window to query
 *   \return              The type of the window
 */
MirWindowType mir_window_get_type(MirWindow* window);

/**
 * Change the state of a window.
 *   \param [in] window  The window to operate on
 *   \param [in] state   The new state of the window
 */
void mir_window_set_state(MirWindow* window,
                                     MirWindowState state);

/**
 * Get the current state of a window.
 *   \param [in] window  The window to query
 *   \return             The state of the window
 */
MirWindowState mir_window_get_state(MirWindow* window);

/**
 * Query the focus state for a window.
 *   \param [in] window  The window to operate on
 *   \return             The focus state of said window
 */
MirWindowFocusState mir_window_get_focus_state(MirWindow* window);

/**
 * Query the visibility state for a window.
 *   \param [in] window  The window to operate on
 *   \return             The visibility state of said window
 */
MirWindowVisibility mir_window_get_visibility(MirWindow* window);

/**
 * Query the DPI value of the window (dots per inch). This will vary depending
 * on the physical display configuration and where the window is within it.
 *   \return  The DPI of the window, or zero if unknown.
 */
int mir_window_get_dpi(MirWindow* window);

/**
 * Choose the cursor state for a window: whether a cursor is shown,
 * and which cursor if so.
 *    \deprecated Users should use mir_window_spec_set_cursor_name/mir_window_spec_set_cursor_render_surface
 *    \param [in] window     The window to operate on
 *    \param [in] parameters The configuration parameters obtained
 *                           from mir_cursor* family of functions.
 *
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void mir_window_configure_cursor(MirWindow* window, MirCursorConfiguration const* parameters)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_spec_set_cursor_name/mir_window_spec_set_cursor_render_surface instead");
#pragma GCC diagnostic pop
/**
 * Request to set the preferred orientations of a window.
 * The request may be rejected by the server; to check wait on the
 * result and check the applied value using mir_window_get_preferred_orientation
 *   \param [in] window      The window to operate on
 *   \param [in] orientation The preferred orientation modes
*/
void mir_window_set_preferred_orientation(MirWindow* window, MirOrientationMode orientation);

/**
 * Get the preferred orientation modes of a window.
 *   \param [in] window   The window to query
 *   \return              The preferred orientation modes
 */
MirOrientationMode mir_window_get_preferred_orientation(MirWindow* window);

/**
 * \brief Request an ID for the window that can be shared cross-process and
 *        across restarts.
 *
 * This call acquires a MirWindowId for this MirWindow. This MirWindowId
 * can be serialized to a string, stored or sent to another process, and then
 * later deserialized to refer to the same window.
 *
 * \param [in]     window    The window to acquire a persistent reference to.
 * \param [in]     callback  Callback to invoke when the request completes.
 * \param [in,out] context   User data passed to completion callback.
 */
void mir_window_request_persistent_id(MirWindow* window, MirWindowIdCallback callback, void* context)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_request_window_id() instead");
void mir_window_request_window_id(MirWindow* window, MirWindowIdCallback callback, void* context);

/**
 * \brief Request a persistent ID for a window and wait for the result
 * \param [in] window  The window to acquire a persistent ID for.
 * \return A MirWindowId. This MirWindowId is owned by the calling code, and must
 *         be freed with a call to mir_persistent_id_release()
 */
MirPersistentId* mir_window_request_persistent_id_sync(MirWindow* window)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_request_window_id_sync");
MirWindowId* mir_window_request_window_id_sync(MirWindow* window);
#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_WINDOW_H_ */
