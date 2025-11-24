/*
 * Copyright © Canonical Ltd.
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
 */

#include <mir_toolkit/cursors.h>

// Cursor names are from CSS3: https://www.w3.org/TR/css-ui-3/#propdef-cursor
extern "C" char const* const mir_default_cursor_name = "default";
extern "C" char const* const mir_disabled_cursor_name = "none";
extern "C" char const* const mir_arrow_cursor_name = "default";
extern "C" char const* const mir_busy_cursor_name = "wait";
extern "C" char const* const mir_caret_cursor_name = "text";
extern "C" char const* const mir_pointing_hand_cursor_name = "pointer";
extern "C" char const* const mir_open_hand_cursor_name = "grab";
extern "C" char const* const mir_closed_hand_cursor_name = "grabbing";
extern "C" char const* const mir_horizontal_resize_cursor_name = "ew-resize";
extern "C" char const* const mir_vertical_resize_cursor_name = "ns-resize";
extern "C" char const* const mir_diagonal_resize_bottom_to_top_cursor_name = "ne-resize";
extern "C" char const* const mir_diagonal_resize_top_to_bottom_cursor_name = "se-resize";
extern "C" char const* const mir_omnidirectional_resize_cursor_name = "move";
extern "C" char const* const mir_vsplit_resize_cursor_name = "row-resize";
extern "C" char const* const mir_hsplit_resize_cursor_name = "col-resize";
extern "C" char const* const mir_crosshair_cursor_name = "crosshair";
