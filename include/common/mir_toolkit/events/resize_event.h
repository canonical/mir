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

#ifndef MIR_TOOLKIT_EVENTS_RESIZE_EVENT_H_
#define MIR_TOOLKIT_EVENTS_RESIZE_EVENT_H_

#include <mir_toolkit/events/event.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Retrieve the new width reported by a given MirResizeEvent
 *
 * \param[in] ev The resize event
 * \return       The reported width
 */
int mir_resize_event_get_width(MirResizeEvent const* ev);

/**
 * Retrieve the new height reported by a given MirResizeEvent
 *
 * \param[in] ev The resize event
 * \return       The reported height
 */
int mir_resize_event_get_height(MirResizeEvent const* ev);

#ifdef __cplusplus
}
#endif

#endif /* MIR_TOOLKIT_RESIZE_EVENT_H_ */
