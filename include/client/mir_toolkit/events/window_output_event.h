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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_TOOLKIT_WINDOW_OUTPUT_EVENT_H_
#define MIR_TOOLKIT_WINDOW_OUTPUT_EVENT_H_

#include <mir_toolkit/events/event.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieve the DPI of the new output configuration of a MirWindowOutputEvent
 *
 * \param [in] ev   The event
 * \return          The new DPI value for the window is primarily on.
 */
int mir_window_output_event_get_dpi(MirWindowOutputEvent const* ev);

/**
 * Retrieve the form factor of the new output configuration of a MirWindowOutputEvent
 *
 * \param [in] ev   The event
 * \return          The new form factor of the output the window is primarily on.
 */
MirFormFactor mir_window_output_event_get_form_factor(MirWindowOutputEvent const* ev);

/**
 * Retrieve the suggested scaling factor of the new output configuration of a
 * MirWindowOutputEvent.
 *
 * \param [in] ev   The event
 * \return          The new scaling factor of the output the window is primarily on.
 */
float mir_window_output_event_get_scale(MirWindowOutputEvent const* ev);

/**
 * Retrieve the maximum refresh rate of the output(s) associated with a
 * MirWindowOutputEvent. For variable refresh rate displays this represents
 * the maximum refresh rate of the display to aim for, rather than a measurement
 * of recent performance.
 *
 * \param [in] ev   The event
 * \return          The refresh rate in Hz
 */
double mir_window_output_event_get_refresh_rate(MirWindowOutputEvent const* ev);

/**
 * Retrieve the ID of the output this window is on from a MirWindowOutputEvent
 *
 * \param [in] ev   The event
 * \return          The ID of the output the window is currently considered to be on.
 *                  (From MirDisplayOutput::output_id)
 */
uint32_t mir_window_output_event_get_output_id(MirWindowOutputEvent const *ev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif //MIR_TOOLKIT_WINDOW_OUTPUT_EVENT_H_
