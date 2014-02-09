/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER mir_server_msgproc

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./message_processor_report_tp.h"

#if !defined(MIR_LTTNG_MESSAGE_PROCESSOR_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_MESSAGE_PROCESSOR_REPORT_TP_H_

#include "lttng_utils.h"

TRACEPOINT_EVENT_CLASS(
    mir_server_msgproc,
    method_event,
    TP_ARGS(const void*, mediator, int, id, const char*, method),
    TP_FIELDS(
        ctf_integer_hex(void*, mediator, mediator)
        ctf_integer(int, id, id)
        ctf_string(method, method)
        )
    )

TRACEPOINT_EVENT_INSTANCE(
    mir_server_msgproc,
    method_event,
    received_invocation,
    TP_ARGS(const void*, mediator, int, id, const char*, method)
    )

TRACEPOINT_EVENT(
    mir_server_msgproc,
    completed_invocation,
    TP_ARGS(const void*, mediator, int, id, int, result),
    TP_FIELDS(
        ctf_integer_hex(void*, mediator, mediator)
        ctf_integer(int, id, id)
        ctf_integer(int, result, result)
        )
    )

TRACEPOINT_EVENT_INSTANCE(
    mir_server_msgproc,
    method_event,
    unknown_method,
    TP_ARGS(const void*, mediator, int, id, char const*, method)
    )


TRACEPOINT_EVENT(
    mir_server_msgproc,
    exception_handled,
    TP_ARGS(const void*, mediator, int, id, char const*, exception),
    TP_FIELDS(
        ctf_integer_hex(void*, mediator, mediator)
        ctf_integer(int, id, id)
        ctf_string(exception, exception)
        )
    )

TRACEPOINT_EVENT(
    mir_server_msgproc,
    exception_handled_wo_invocation,
    TP_ARGS(const void*, mediator, char const*, exception),
    TP_FIELDS(
        ctf_integer_hex(void*, mediator, mediator)
        ctf_string(exception, exception)
        )
    )

TRACEPOINT_EVENT(
    mir_server_msgproc,
    sent_event,
    TP_ARGS(const void*, mediator, int, surface_id, int, attribute, int, value),
    TP_FIELDS(
        ctf_integer_hex(void*, mediator, mediator)
        ctf_integer(int, surface_id, surface_id)
        ctf_integer(int, attribute, attribute)
        ctf_integer(int, value, value)
        )
    )

#include "lttng_utils_pop.h"

#endif /* MIR_LTTNG_MESSAGE_PROCESSOR_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
