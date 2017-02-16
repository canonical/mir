/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by:
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#ifndef MIR_TOOLKIT_MIR_RENDER_SURFACE_H_
#define MIR_TOOLKIT_MIR_RENDER_SURFACE_H_

#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
/**
 * Create a render surface
 *
 * \param [in] connection                       A valid connection
 * \param [in] width                            The width in pixels
 * \param [in] height                           The height in pixels
 * \param [in] MirRenderSurfaceCallback         Callback to be invoked when the request completes.
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
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

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
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

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
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

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
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

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
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

/**
 * Retrieve a text description of the error. The returned string is owned by
 * the library and remains valid until the render surface or the associated
 * connection has been released.
 *   \param [in] redner_surface  The render surface
 *   \return              A text description of any error resulting in an
 *                        invalid render surface, or the empty string "" if the
 *                        object is valid.
 */
char const *mir_render_surface_get_error_message(
    MirRenderSurface* render_surface)
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

/**
 * Release the specified render surface
 *
 * \param [in] render_surface                   The render surface to be released
 */
void mir_render_surface_release(
    MirRenderSurface* render_surface)
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

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
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

/**
 * Obtain the presentation chain backing a given render surface
 *
 * \return                       The chain contained in the given render surface
 *                               or 'nullptr' if it, or
 *                               mir_render_surface_get_buffer_stream(), has already
 *                               been called once
 */
MirPresentationChain* mir_render_surface_get_presentation_chain(
    MirRenderSurface* render_surface)
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

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
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

MirCursorConfiguration* mir_cursor_configuration_from_render_surface(
    MirRenderSurface* surface,
    int hotspot_x, int hotspot_y)
__attribute__((deprecated("This function is slated for rename due to MirRenderSurface-->MirSurface transition")));

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_MIR_RENDER_SURFACE_H_
