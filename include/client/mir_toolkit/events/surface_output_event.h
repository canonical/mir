/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TOOLKIT_SURFACE_OUTPUT_EVENT_H_
#define MIR_TOOLKIT_SURFACE_OUTPUT_EVENT_H_

#include <mir_toolkit/events/event.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieve the DPI of the new output configuration of a MirSurfaceOutputEvent
 *
 * \param [in] ev   The event
 * \return          The new DPI value for the surface is primarily on.
 */
int mir_surface_output_event_get_dpi(MirSurfaceOutputEvent const* ev);

/**
 * Retrieve the form factor of the new output configuration of a MirSurfaceOutputEvent
 *
 * \param [in] ev   The event
 * \return          The new form factor of the output the surface is primarily on.
 */
MirFormFactor mir_surface_output_event_get_form_factor(MirSurfaceOutputEvent const* ev);

/**
 * Retrieve the form factor of the new output configuration of a MirSurfaceOutputEvent
 *
 * \param [in] ev   The event
 * \return          The new form factor of the output the surface is primarily on.
 */
float mir_surface_output_event_get_scale(MirSurfaceOutputEvent const* ev);

/**
 * Retrieve the ID of the output this surface is on from a MirSurfaceOutputEvent
 *
 * \param [in] ev   The event
 * \return          The ID of the output the surface is currently considered to be on.
 *                  (From MirDisplayOutput::output_id)
 */
uint32_t mir_surface_output_event_get_output_id(MirSurfaceOutputEvent const *ev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif //MIR_TOOLKIT_SURFACE_OUTPUT_EVENT_H_
