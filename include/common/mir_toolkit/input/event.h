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

#ifndef MIR_TOOLKIT_INPUT_EVENT_H_
#define MIR_TOOLKIT_INPUT_EVENT_H_

#include "mir_toolkit/event.h"

#ifdef __cplusplus
/**
 * \addtogroup mir_toolkit
 * @{
 */
extern "C" {
#endif

typedef struct _MirInputEvent MirInputEvent;

typedef enum {
    mir_input_event_type_key,
    mir_input_event_type_touch,
    mir_input_event_type_pointer,
    mir_input_event_type_invalid
} MirInputEventType;

// TODO: Comment about location on these two
MirEventType mir_event_get_type(MirEvent const* ev);
MirInputEvent* mir_event_get_input_event(MirEvent* ev);

MirInputEventType mir_input_event_get_type(MirInputEvent const* ev);

#ifdef __cplusplus
}
/**@}*/
#endif

#endif // MIR_TOOLKIT_INPUT_EVENT_H_
