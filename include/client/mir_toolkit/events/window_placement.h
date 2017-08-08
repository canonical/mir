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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_TOOLKIT_WINDOW_PLACEMENT_H_
#define MIR_TOOLKIT_WINDOW_PLACEMENT_H_

#include <mir_toolkit/client_types.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieve the relative position from a placement notification
 *
 * \param [in] event  The placement event
 * \return            The position relative to the parent window
 */
MirRectangle mir_window_placement_get_relative_position(MirWindowPlacementEvent const* event);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif //MIR_TOOLKIT_WINDOW_PLACEMENT_H_
