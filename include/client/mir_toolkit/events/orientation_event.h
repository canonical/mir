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

#ifndef MIR_TOOLKIT_EVENTS_ORIENTATION_EVENT_H_
#define MIR_TOOLKIT_EVENTS_ORIENTATION_EVENT_H_

#include <mir_toolkit/events/event.h>

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

/**
 * Retrieve the new orientation reported by this MirOrientationEvent
 *
 * \param[in] ev The orientation event
 * \return       The new orientation
 */
MirOrientation mir_orientation_event_get_direction(MirOrientationEvent const* ev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif /* MIR_TOOLKIT_ORIENTATION_EVENT_H_ */
