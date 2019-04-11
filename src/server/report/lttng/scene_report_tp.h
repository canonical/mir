/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER mir_server_scene

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./scene_report_tp.h"

#if !defined(MIR_LTTNG_SCENE_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_SCENE_REPORT_TP_H_

#include <lttng/tracepoint.h>
#include <stdint.h>

#include "lttng_utils.h"

#undef SCENE_TRACE_POINT

TRACEPOINT_EVENT_CLASS(
    mir_server_scene,
    surface_event,
    TP_ARGS(char const*, name),
    TP_FIELDS(ctf_string(name, name))
)

TRACEPOINT_EVENT_INSTANCE(
    mir_server_scene,
    surface_event,
    surface_created,
    TP_ARGS(char const*, name)
)

TRACEPOINT_EVENT_INSTANCE(
    mir_server_scene,
    surface_event,
    surface_added,
    TP_ARGS(char const*, name)
)

TRACEPOINT_EVENT_INSTANCE(
    mir_server_scene,
    surface_event,
    surface_removed,
    TP_ARGS(char const*, name)
)

TRACEPOINT_EVENT_INSTANCE(
    mir_server_scene,
    surface_event,
    surface_deleted,
    TP_ARGS(char const*, name)
)

#endif /* MIR_LTTNG_SCENE_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
