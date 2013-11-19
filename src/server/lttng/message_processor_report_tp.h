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

#include <lttng/tracepoint.h>

#ifdef __clang__
/*
 * TRACEPOINT_EVENT defines functions; since we disable tracepoints under clang
 * these functions are unused and so generate fatal warnings.
 * (see mir_tracepoint.h and http://sourceware.org/bugzilla/show_bug.cgi?id=13974)
 */
#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wunused-function"
#endif

TRACEPOINT_EVENT(
    mir_server_msgproc,
    received_invocation,
    TP_ARGS(const void*, mediator, int, id, const char*, method),
    TP_FIELDS(
        ctf_integer_hex(void*, mediator, mediator)
        ctf_integer(int, id, id)
        ctf_string(method, method)
    )
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

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif /* MIR_LTTNG_MESSAGE_PROCESSOR_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
