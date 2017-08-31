/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by:
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_TOOLKIT_MIR_RENDER_SURFACE_H_
#define MIR_TOOLKIT_MIR_RENDER_SURFACE_H_

#include <mir_toolkit/client_types.h>
#include <mir_toolkit/deprecations.h>

#ifndef MIR_DEPRECATE_RENDERSURFACES
    #define MIR_DEPRECATE_RENDERSURFACES 0
#endif

#if MIR_ENABLE_DEPRECATIONS > 0 && MIR_DEPRECATE_RENDERSURFACES > 0
    #define MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME\
    __attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")))
#else
    #define MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME
#endif

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

typedef void (*MirRenderSurfaceCallback)(MirRenderSurface*, void* context)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/**
 * Create a render surface
 *
 * \param [in] connection                       A valid connection
 * \param [in] width                            The width in pixels
 * \param [in] height                           The height in pixels
 * \param [in] callback                         Callback to be invoked when the request completes.
 *                                              The callback is guaranteed to be called and called
 *                                              with a non-null MirRenderSurface*, but the render
 *                                              surface may be invalid in case of error.
 * \param [in,out] context                      User data to pass to callback function
 */
void mir_connection_create_render_surface(
    MirConnection* connection,
    int width, int height,
    MirRenderSurfaceCallback callback,
    void* context)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Create a render surface and wait for the result
 *
 * \param [in] connection       A valid connection
 * \param [in] width            The width in pixels
 * \param [in] height           The height in pixels
 *
 * \return                      The new render surface, guaranteed to be
 *                              non-null, but may be invalid in case of error
 */
MirRenderSurface* mir_connection_create_render_surface_sync(
    MirConnection* connection,
    int width, int height)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Get the size of the MirRenderSurface
 *
 * \param [in] render_surface   The render surface
 * \param [out] width           The width in pixels
 * \param [out] height          The height in pixels
 */ 
void mir_render_surface_get_size(
    MirRenderSurface* render_surface,
    int* width, int* height)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Set the size of the MirRenderSurface
 *
 * \param [in] render_surface   The render surface
 * \param [in] width           The width in pixels
 * \param [in] height          The height in pixels
 */ 
void mir_render_surface_set_size(
    MirRenderSurface* render_surface,
    int width, int height)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Test for a valid render surface
 *
 * \param [in] render_surface  The render surface
 *
 * \return                     True if the supplied render surface is valid,
 *                             or false otherwise
 */
bool mir_render_surface_is_valid(
    MirRenderSurface* render_surface)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Retrieve a text description of the error. The returned string is owned by
 * the library and remains valid until the render surface or the associated
 * connection has been released.
 *   \param [in] render_surface  The render surface
 *   \return              A text description of any error resulting in an
 *                        invalid render surface, or the empty string "" if the
 *                        object is valid.
 */
char const *mir_render_surface_get_error_message(
    MirRenderSurface* render_surface)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Release the specified render surface
 *
 * \param [in] render_surface                   The render surface to be released
 */
void mir_render_surface_release(
    MirRenderSurface* render_surface)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Obtain the buffer stream backing a given render surface.
 * The MirBufferStream will contain buffers suitable for writing via the CPU. 
 *
 * \param [in] render_surface    The render surface
 * \param [in] width             Requested width
 * \param [in] height            Requested height
 * \param [in] format            Requested pixel format
 *
 * \return                       The buffer stream contained in the given render surface
 *                               or 'nullptr' if it, or
 *                               mir_render_surface_get_presentation_chain(), has already
 *                               been called once
 */
MirBufferStream* mir_render_surface_get_buffer_stream(
    MirRenderSurface* render_surface,
    int width, int height,
    MirPixelFormat format)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Obtain the presentation chain backing a given render surface.
 * The MirPresentationChain is created in mir_present_mode_fifo submission mode.
 *
 * \return                       The chain contained in the given render surface
 *                               or 'nullptr' if it, or
 *                               mir_render_surface_get_buffer_stream(), has already
 *                               been called once
 */
MirPresentationChain* mir_render_surface_get_presentation_chain(
    MirRenderSurface* render_surface)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/** Query whether the server supports a given presentation mode.
 *
 *  \param [in] connection  The connection
 *  \param [in] mode        The MirPresentMode
 *  \return                 True if supported, false if not
 */
bool mir_connection_present_mode_supported(
    MirConnection* connection, MirPresentMode mode);

/** Respecify the submission mode that the MirPresentationChain is operating with.
 *  The buffers currently queued will immediately be requeued according
 *  to the new mode.
 *
 *  \pre    mir_connection_present_mode_supported must indicate that the mode is supported
 *  \param [in] chain   The chain
 *  \param [in] mode    The mode to change to
 */
void mir_presentation_chain_set_mode(
    MirPresentationChain* chain, MirPresentMode mode);

/**
 * Set the MirWindowSpec to contain a specific cursor.
 *
 * \param [in] spec             The spec
 * \param [in] render_surface   The rendersurface to set, or nullptr to reset to default cursor.
 * \param [in] hotspot_x        The x-coordinate to use as the cursor's hotspot
 * \param [in] hotspot_y        The y-coordinate to use as the cursor's hotspot
 */
void mir_window_spec_set_cursor_render_surface(
    MirWindowSpec* spec,
    MirRenderSurface* render_surface,
    int hotspot_x, int hotspot_y)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

/**
 * Returns a new cursor configuration tied to a given render surface.
 * If the configuration is successfully applied buffers from the surface
 * will be used to fill the system cursor.
 *    \param [in] surface      The render surface
 *    \param [in] hotspot_x The x-coordinate to use as the cursor's hotspot.
 *    \param [in] hotspot_y The y-coordinate to use as the cursor's hotspot.
 *    \return A cursor parameters object which must be passed
 *            to_mir_cursor_configuration_destroy
 */
MirCursorConfiguration* mir_cursor_configuration_from_render_surface(
    MirRenderSurface* surface,
    int hotspot_x, int hotspot_y)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;


/**
 * Set the MirWindowSpec to display content contained in a render surface
 *
 * \warning: The initial call to mir_window_spec_add_render_surface will set
 *           the bottom-most content, and subsequent calls will stack the
 *           content on top.
 *
 * \param spec             The window_spec to be updated
 * \param render_surface   The render surface containing the content to be displayed
 * \param logical_width    The width that the content will be displayed at
 *                         (Ignored for buffer streams)
 * \param logical_height   The height that the content will be displayed at
 *                         (Ignored for buffer streams)
 * \param displacement_x   The x displacement from the top-left corner of the MirWindow
 * \param displacement_y   The y displacement from the top-left corner of the MirWindow
 */
void mir_window_spec_add_render_surface(MirWindowSpec* spec,
                                        MirRenderSurface* render_surface,
                                        int logical_width, int logical_height,
                                        int displacement_x, int displacement_y)
MIR_DEPRECATE_RENDERSURFACES_FOR_RENAME;

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_RENDER_SURFACE_H_
