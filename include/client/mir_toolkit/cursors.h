/*
 * Cursor name definitions.
 *
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_CURSORS_H_
#define MIR_CURSORS_H_

/**
 * \addtogroup mir_toolkit
 * @{
 */

/* This is C code. Not C++. */
#ifdef __cplusplus
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
 * The standard arrow cursor (typically the system default)
 */
extern char const* const mir_arrow_cursor_name;

/**
 * The "wait" cursor, typically an hourglass or watch used during operations
 * which prevent the user from interacting.
 */
extern char const* const mir_busy_cursor_name;

/**
 * The caret or ibeam cursor, indicating acceptance of text input
 */
extern char const* const mir_caret_cursor_name;

/**
 * The pointing hand cursor, typically used for clickable elements such
 * as hyperlinks.
 */
extern char const* const mir_pointing_hand_cursor_name;

/**
 * The open handed cursor, typically used to indicate that the area beneath
 * the cursor may be clicked and dragged around.
 */
extern char const* const mir_open_hand_cursor_name;

/**
 * The close handed cursor, typically used to indicate that a drag operation is in process
 * which involves scrolling.
 */
extern char const* const mir_closed_hand_cursor_name;

/**
 * The cursor used to indicate a horizontal resize operation.
 */
extern char const* const mir_horizontal_resize_cursor_name;

/**
 * The cursor used to indicate a vertical resize operation.
 */
extern char const* const mir_vertical_resize_cursor_name;

/**
 * The cursor used to indicate diagonal resize from top-right and bottom-left corners.
 */
extern char const* const mir_diagonal_resize_bottom_to_top_cursor_name;

/**
 * The cursor used to indicate diagonal resize from bottom-left and top-right corners.
 */
extern char const* const mir_diagonal_resize_top_to_bottom_cursor_name;

/**
 * The cursor used to indicate resize with no directional constraint.
 */
extern char const* const mir_omnidirectional_resize_cursor_name;

/**
 * The cursor used for vertical splitters, indicating that a handle may be
 * dragged to adjust vertical space.
 */
extern char const* const mir_vsplit_resize_cursor_name;

/**
 * The cursor used for horizontal splitters, indicating that a handle may be
 * dragged to adjust horizontal space.
 */
extern char const* const mir_hsplit_resize_cursor_name;

/**
 *  The cursor used for crosshair, which may be used for picking colors or
 *  finer precision.
 */
extern char const* const mir_crosshair_cursor_name;

#ifdef __cplusplus
}
#endif
/**@}*/

#endif
