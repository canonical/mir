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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#ifndef MIR_TOOLKIT_EVENTS_WINDOW_EVENT_H_
#define MIR_TOOLKIT_EVENTS_WINDOW_EVENT_H_

#include <mir_toolkit/events/event.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieve the attribute index configured with a given MirWindowEvent
 *
 * \param [in] event The event
 * \return           The associated attribute
 */
MirWindowAttrib mir_window_event_get_attribute(MirWindowEvent const* event);

/**
 * Retrieve the new value of the associated attribute for a given MirWindowEvent
 *
 * \param [in] event The event
 * \return           The associated attribute value
 */
int mir_window_event_get_attribute_value(MirWindowEvent const* event);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_WINDOW_EVENT_H_ */
