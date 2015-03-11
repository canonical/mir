/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_TOOLKIT_MIR_CURSOR_H_
#define MIR_TOOLKIT_MIR_CURSOR_H_

#include <mir_toolkit/common.h>
#include <mir_toolkit/client_types.h>

/**
 * Opaque structure containing cursor parameterization. Create with mir_cursor* family.
 * Used with mir_surface_configure_cursor.
 */
typedef struct MirCursorConfiguration MirCursorConfiguration;

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Release resources assosciated with cursor parameters
 *     \param [in] parameters The operand
 */
void mir_cursor_configuration_destroy(MirCursorConfiguration *parameters);

/**
 * Returns a new MirCursorConfiguration representing a named cursor
 * from the system cursor theme. Symbolic cursor names, such as
 * mir_default_cursor_name and mir_caret_cursor_name are available
 * see (mir_toolkit/cursors.h).
 * as input.
 *    \param [in] name The cursor name
 *    \return A cursor parameters object which must be passed
 *            to_mir_cursor_configuration_destroy
 */
MirCursorConfiguration *mir_cursor_configuration_from_name(char const* name);

/**
 * Returns a new cursor configuration tied to a given buffer stream.
 * If the configuration is successfully applied buffers from the stream will be used 
 * to fill the system cursor.
 *    \param [in] name      The buffer stream
 *    \param [in] hotspot_x The x-coordinate to use as the cursor's hotspot.
 *    \param [in] hotspot_y The y-coordinate to use as the cursor's hotspot.
 *    \return A cursor parameters object which must be passed
 *            to_mir_cursor_configuration_destroy
 */
MirCursorConfiguration *mir_cursor_configuration_from_buffer_stream(MirBufferStream const* stream, int hotspot_x,
    int hotspot_y);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_CURSOR_H_ */
