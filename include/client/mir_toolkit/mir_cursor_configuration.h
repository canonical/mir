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
 * A special cursor name for use with mir_cursor_configuration_from_name
 * representing the system default cursor.
 */
extern char const *const mir_default_cursor_name;
/**
 * A special cursor name for use with mir_cursor_configuration_from_name
 * representing a disabled cursor image.
 */
extern char const *const mir_disabled_cursor_name;

/**
 * Release resources assosciated with cursor parameters
 *     \param [in] parameters The operand
 */
void mir_cursor_configuration_destroy(MirCursorConfiguration *parameters);

/**
 * Returns a new MirCursorConfiguration representing a named cursor
 * from the system cursor theme. Currently only the symbolic values,
 * mir_default_cursor_name and mir_disabled_cursor_name are supported
 * as input.
 *    \param [in] name The cursor name
 *    \return A cursor parameters object which must be passed
 *            to_mir_cursor_configuration_destroy
 */
MirCursorConfiguration *mir_cursor_configuration_from_name(char const* name);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_MIR_CURSOR_H_ */
