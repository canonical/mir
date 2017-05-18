/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_CLIENT_LIBRARY_DEBUG_H
#define MIR_CLIENT_LIBRARY_DEBUG_H

#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/deprecations.h>

/* This header defines debug interfaces that aren't expected to be generally useful
 * and do not have the same API-stability guarantees that the main API has */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Return the ID of a window (only useful for debug output).
 *   \pre                 The window is valid
 *   \param [in] window   The window
 *   \return              An internal ID that identifies the window
 */
int mir_debug_window_id(MirWindow *window);

/**
 * Get the ID of the window's current buffer (only useful for debug purposes)
 *   \pre                         The window is valid
 *   \param   [in] window         The window
 *   \return                      The internal buffer ID of the window's current buffer.
 *                                This is the buffer that is currently being drawn to,
 *                                and would be returned by mir_window_get_current_buffer.
 */
uint32_t mir_debug_window_current_buffer_id(MirWindow *window);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

/**
 * Get the screen coordinates corresponding to a pair of surface coordinates
 * \pre                               The surface is valid
 * \param   [in] surface              The surface
 * \param   [in] x, y                 Surface coordinates to map to screen coordinates
 * \param   [out] screen_x, screen_y  The screen coordinates corresponding to x, y.
 * \return                            True if screen_x and screen_y contain values
 * \note    There are many cases where such a mapping does not exist or would be expensive
 *          to calculate. Only Mir servers started with the --debug option will ever return
 *          values for this call, and even when --debug is enabled servers are free to
 *          return nothing.
 *
 *          This call will only be interesting for automated testing, where both the client
 *          and shell state is known and constrained.
 */
bool mir_debug_surface_coords_to_screen(MirSurface *surface,
                                       int x, int y,
                                       int* screen_x, int* screen_y)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_extension_window_coordinate_translation instead");

int mir_debug_surface_id(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_debug_window_id() instead");

uint32_t mir_debug_surface_current_buffer_id(MirSurface *surface)
MIR_FOR_REMOVAL_IN_VERSION_1("Use mir_debug_window_current_buffer_id() instead");

#pragma GCC diagnostic pop

#ifdef __cplusplus
}
#endif

#endif /* MIR_CLIENT_LIBRARY_DEBUG_H */
