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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TOOLKIT_SURFACE_PLACEMENT_H_
#define MIR_TOOLKIT_SURFACE_PLACEMENT_H_

#include <mir_toolkit/client_types.h>
#include <mir_toolkit/deprecations.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

typedef struct MirSurfacePlacementEvent MirSurfacePlacementEvent;

/**
 * Retrieve the relative position from a placement notification
 * 
 * \param [in] event  The placement event
 * \return            The position relative to the parent surface
 */
MirRectangle mir_surface_placement_get_relative_position(MirSurfacePlacementEvent const* event)
MIR_FOR_REMOVAL_IN_VERSION_1("use mir_window_placement_get_relative_position instead");

#ifdef __cplusplus
}
/**@}*/
#endif

#endif //MIR_TOOLKIT_SURFACE_PLACEMENT_H_
