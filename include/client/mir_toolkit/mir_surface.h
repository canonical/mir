/*
 * Copyright © 2012-2016 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_MIR_SURFACE_H_
#define MIR_TOOLKIT_MIR_SURFACE_H_

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

// Functions in this pragma section are to be deprecated
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

MirSurfaceSpec* mir_connection_create_spec_for_normal_surface(MirConnection* connection,
                                                              int width, int height,
                                                              MirPixelFormat format)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_create_normal_window_spec() instead");

MirSurfaceSpec*
mir_connection_create_spec_for_menu(MirConnection* connection,
                                    int width,
                                    int height,
                                    MirPixelFormat format,
                                    MirSurface* parent,
                                    MirRectangle* rect,
                                    MirEdgeAttachment edge)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_specify_menu() instead");

MirSurfaceSpec*
mir_connection_create_spec_for_tooltip(MirConnection* connection,
                                       int width, int height,
                                       MirPixelFormat format,
                                       MirSurface* parent,
                                       MirRectangle* zone)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_create_tip_window_spec() instead");

MirSurfaceSpec*
mir_connection_create_spec_for_tip(MirConnection* connection,
                                   int width, int height,
                                   MirPixelFormat format,
                                   MirSurface* parent,
                                   MirRectangle* rect,
                                   MirEdgeAttachment edge)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_create_tip_window_spec() instead");

MirSurfaceSpec*
mir_connection_create_spec_for_modal_dialog(MirConnection* connection,
                                            int width, int height,
                                            MirPixelFormat format,
                                            MirSurface* parent)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_create_modal_dialog_window_spec() instead");

MirSurfaceSpec*
mir_connection_create_spec_for_dialog(MirConnection* connection,
                                      int width, int height,
                                      MirPixelFormat format)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_create_dialog_window_spec() instead");

MirSurfaceSpec* mir_create_surface_spec(MirConnection* connection)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_create_window_spec() instead");

MirSurfaceSpec*
mir_connection_create_spec_for_changes(MirConnection* connection)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_create_window_spec() instead");

void mir_surface_spec_set_parent(MirSurfaceSpec* spec, MirSurface* parent)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_window_spec_set_parent() instead");

/**
 *\deprecated This will soon be a property of the backing content.
 *
 * Query the swapinterval that the surface is operating with.
 * The default interval is 1.
 *   \param [in] surface  The surface to operate on
 *   \return              The swapinterval value that the client is operating with.
 *                        Returns -1 if surface is invalid, or if the default stream
 *                        was removed by use of mir_window_spec_set_streams().
 */
int mir_surface_get_swapinterval(MirSurface* surface)
MIR_FOR_REMOVAL_IN_VERSION_1("This will soon be a property of the backing content");

void mir_surface_spec_set_type(MirSurfaceSpec* spec, MirSurfaceType type)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_type() instead");

void mir_surface_spec_set_name(MirSurfaceSpec* spec, char const* name)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_name() instead");

void mir_surface_spec_set_width(MirSurfaceSpec* spec, unsigned width)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_width() instead");

void mir_surface_spec_set_height(MirSurfaceSpec* spec, unsigned height)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_height() instead");

void mir_surface_spec_set_width_increment(MirSurfaceSpec* spec, unsigned width_inc)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_width_increment() instead");

void mir_surface_spec_set_height_increment(MirSurfaceSpec* spec, unsigned height_inc)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_height_increment() instead");

void mir_surface_spec_set_min_width(MirSurfaceSpec* spec, unsigned min_width)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_min_width() instead");

void mir_surface_spec_set_min_height(MirSurfaceSpec* spec, unsigned min_height)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_min_height() instead");

void mir_surface_spec_set_max_width(MirSurfaceSpec* spec, unsigned max_width)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_max_width() instead");

void mir_surface_spec_set_max_height(MirSurfaceSpec* spec, unsigned max_height)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_max_height() instead");

void mir_surface_spec_set_min_aspect_ratio(MirSurfaceSpec* spec, unsigned width, unsigned height)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_min_aspect_ratio() instead");

void mir_surface_spec_set_max_aspect_ratio(MirSurfaceSpec* spec, unsigned width, unsigned height)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_max_aspect_ratio() instead");

void mir_surface_spec_set_fullscreen_on_output(MirSurfaceSpec* spec, uint32_t output_id)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_fullscreen_on_output() instead");

void mir_surface_spec_set_preferred_orientation(MirSurfaceSpec* spec, MirOrientationMode mode)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_preferred_orientation() instead");

bool mir_surface_spec_attach_to_foreign_parent(MirSurfaceSpec* spec,
                                               MirPersistentId* parent,
                                               MirRectangle* attachment_rect,
                                               MirEdgeAttachment edge)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_attach_to_foreign_parent() instead");

void mir_surface_spec_set_state(MirSurfaceSpec* spec, MirSurfaceState state)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_state() instead");

void mir_surface_spec_release(MirSurfaceSpec* spec)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_release() instead");

void mir_surface_spec_set_input_shape(MirSurfaceSpec* spec,
                                      MirRectangle const *rectangles,
                                      size_t n_rects)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_input_shape() instead");

void mir_surface_spec_set_event_handler(MirSurfaceSpec* spec,
                                        mir_surface_event_callback callback,
                                        void* context)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_event_handler() instead");

void mir_surface_spec_set_shell_chrome(MirSurfaceSpec* spec, MirShellChrome style)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_shell_chrome() instead");

void mir_surface_spec_set_pointer_confinement(MirSurfaceSpec* spec, MirPointerConfinementState state)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_pointer_confinement() instead");

void mir_surface_spec_set_placement(MirSurfaceSpec*     spec,
                                    const MirRectangle* rect,
                                    MirPlacementGravity rect_gravity,
                                    MirPlacementGravity window_gravity,
                                    MirPlacementHints   placement_hints,
                                    int                 offset_dx,
                                    int                 offset_dy)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_placement() instead");

MirSurfaceSpec* mir_connection_create_spec_for_input_method(MirConnection* connection,
                                                            int width, int height,
                                                            MirPixelFormat format)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_create_input_method_window_spec() instead");

void mir_surface_spec_set_pixel_format(MirSurfaceSpec* spec, MirPixelFormat format)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_pixel_format() instead");

void mir_surface_spec_set_buffer_usage(MirSurfaceSpec* spec, MirBufferUsage usage)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_buffer_usage() instead");

void mir_surface_spec_set_streams(MirSurfaceSpec* spec,
                                  MirBufferStreamInfo* streams,
                                  unsigned int num_streams)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_spec_set_streams() instead");

void mir_surface_apply_spec(MirSurface* surface, MirSurfaceSpec* spec)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_apply_spec() instead");

bool mir_surface_is_valid(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_is_valid() instead");

MirWaitHandle* mir_surface_create(MirSurfaceSpec* requested_specification,
                                  mir_surface_callback callback, void* context)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_create_window() instead");

MirSurface* mir_surface_create_sync(MirSurfaceSpec* requested_specification)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_create_window_sync() instead");

MirWaitHandle *mir_surface_release(
    MirSurface *surface,
    mir_surface_callback callback,
    void *context)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_release() instead");

void mir_surface_release_sync(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_release_sync() instead");

void mir_surface_set_event_handler(MirSurface *surface,
                                   mir_surface_event_callback callback,
                                   void* context)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_set_event_handler() instead");

MirBufferStream* mir_surface_get_buffer_stream(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_buffer_stream() instead");

char const* mir_surface_get_error_message(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_error_message() instead");

void mir_surface_get_parameters(MirSurface *surface, MirSurfaceParameters *parameters)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_parameters() instead");

MirSurfaceType mir_surface_get_type(MirSurface* surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_type() instead");

MirWaitHandle* mir_surface_set_state(MirSurface *surface,
                                     MirSurfaceState state)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_set_state() instead");

MirSurfaceState mir_surface_get_state(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_state() instead");

/**
 * Set the swapinterval for the default stream.
 *   \warning EGL users should use eglSwapInterval directly.
 *   \warning If the surface was created with, or modified to have a
 *            MirSurfaceSpec containing streams added through
 *            mir_window_spec_set_streams(), the default stream will
 *            be removed, and this function will return NULL.
 *   \param [in] surface  The surface to operate on
 *   \param [in] interval The number of vblank signals that
 *                        mir_surface_swap_buffers will wait for
 *   \return              A wait handle that can be passed to mir_wait_for,
 *                        or NULL if the interval could not be supported
 */
MirWaitHandle* mir_surface_set_swapinterval(MirSurface* surface, int interval)
MIR_FOR_REMOVAL_IN_VERSION_1("Swap interval should be set on the backing content");

int mir_surface_get_dpi(MirSurface* surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_dpi() instead");

MirSurfaceFocusState mir_surface_get_focus(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_focus_state() instead");

MirSurfaceVisibility mir_surface_get_visibility(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_visibility() instead");

MirWaitHandle* mir_surface_configure_cursor(MirSurface *surface, MirCursorConfiguration const* parameters)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_configure_cursor() instead");

MirOrientation mir_surface_get_orientation(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_orientation() instead");

MirWaitHandle* mir_surface_set_preferred_orientation(MirSurface *surface, MirOrientationMode orientation)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_set_preferred_orientation() instead");

MirOrientationMode mir_surface_get_preferred_orientation(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_get_preferred_orientation() instead");

MirWaitHandle* mir_surface_request_persistent_id(MirSurface* surface, mir_surface_id_callback callback, void* context)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_request_persistent_id() instead");

MirPersistentId* mir_surface_request_persistent_id_sync(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_request_persistent_id_sync() instead");

void mir_surface_raise(MirSurface* surface, MirCookie const* cookie)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_raise() instead");

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_SURFACE_H_ */
