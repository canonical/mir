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
#define TRACEPOINT_PROVIDER mir_server_compositor

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./compositor_report_tp.h"

#if !defined(MIR_LTTNG_COMPOSITOR_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_COMPOSITOR_REPORT_TP_H_

#include "lttng_utils.h"

MIR_LTTNG_VOID_TRACE_CLASS(mir_server_compositor)

#define COMPOSITOR_TRACE_POINT(name) MIR_LTTNG_VOID_TRACE_POINT(TRACEPOINT_PROVIDER, name)

COMPOSITOR_TRACE_POINT(started)
COMPOSITOR_TRACE_POINT(stopped)
COMPOSITOR_TRACE_POINT(scheduled)

#undef COMPOSITOR_TRACE_POINT

TRACEPOINT_EVENT(
    mir_server_compositor,
    added_display,
    TP_ARGS(int, width, int, height, int, x, int, y, void const*, id),
    TP_FIELDS(
        ctf_integer(int, width, width)
        ctf_integer(int, height, height)
        ctf_integer(int, x, x)
        ctf_integer(int, y, y)
        ctf_integer_hex(uintptr_t, id, (uintptr_t)(id))
    )
)

TRACEPOINT_EVENT_CLASS(
    mir_server_compositor,
    subcompositor_event,
    TP_ARGS(void const*, id),
    TP_FIELDS(
        ctf_integer_hex(uintptr_t, id, (uintptr_t)(id))
    )
)

TRACEPOINT_EVENT_INSTANCE(
    mir_server_compositor,
    subcompositor_event,
    began_frame,
    TP_ARGS(void const*, id)
)

TRACEPOINT_EVENT_INSTANCE(
    mir_server_compositor,
    subcompositor_event,
    rendered_frame,
    TP_ARGS(void const*, id)
)

TRACEPOINT_EVENT_INSTANCE(
    mir_server_compositor,
    subcompositor_event,
    finished_frame,
    TP_ARGS(void const*, id)
)

TRACEPOINT_EVENT(
    mir_server_compositor,
    buffers_in_frame,
    TP_ARGS(void const*, id, unsigned int*, buffer_ids, size_t, buffer_ids_len),
    TP_FIELDS(
        ctf_integer_hex(uintptr_t, id, (uintptr_t)(id))
        ctf_sequence(unsigned int, buffer_ids, buffer_ids, size_t, buffer_ids_len)
    )
)

#endif /* MIR_LTTNG_COMPOSITOR_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
