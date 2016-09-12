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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TOOLKIT_EVENTS_SURFACE_EVENT_H_
#define MIR_TOOLKIT_EVENTS_SURFACE_EVENT_H_

#include <mir_toolkit/events/event.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieve the attribute index configured with a given MirSurfaceEvent
 *
 * \param [in] event The event
 * \return           The associated attribute
 */
MirSurfaceAttrib mir_surface_event_get_attribute(MirSurfaceEvent const* event);

/**
 * Retrieve the new value of the associated attribute for a given MirSurfaceEvent
 *
 * \param [in] event The event
 * \return           The associated attribute value
 */
int mir_surface_event_get_attribute_value(MirSurfaceEvent const* event);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_SURFACE_EVENT_H_ */
