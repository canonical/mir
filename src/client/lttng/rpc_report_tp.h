/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER mir_client_rpc

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./rpc_report_tp.h"

#if !defined(MIR_CLIENT_LTTNG_RPC_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_CLIENT_LTTNG_RPC_REPORT_TP_H_

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
    mir_client_rpc,
    invocation_requested,
    TP_ARGS(uint32_t, id, const char*, method),
    TP_FIELDS(
        ctf_integer(uint32_t, id, id)
        ctf_string(method, method)
    )
)

TRACEPOINT_EVENT(
    mir_client_rpc,
    invocation_succeeded,
    TP_ARGS(uint32_t, id, const char*, method),
    TP_FIELDS(
        ctf_integer(uint32_t, id, id)
        ctf_string(method, method)
    )
)

TRACEPOINT_EVENT(
    mir_client_rpc,
    result_receipt_succeeded,
    TP_ARGS(uint32_t, id),
    TP_FIELDS(
        ctf_integer(uint32_t, id, id)
    )
)

TRACEPOINT_EVENT(
    mir_client_rpc,
    event_parsing_succeeded,
    TP_ARGS(int, dummy),
    TP_FIELDS(
        ctf_integer(int, dummy, dummy)
    )
)

TRACEPOINT_EVENT(
    mir_client_rpc,
    complete_response,
    TP_ARGS(uint32_t, id),
    TP_FIELDS(
        ctf_integer(uint32_t, id, id)
    )
)

TRACEPOINT_EVENT(
    mir_client_rpc,
    file_descriptors_received,
    TP_ARGS(const int32_t*, fds, size_t, len),
    TP_FIELDS(
        ctf_sequence(int32_t, fds, fds, size_t, len)
    )
)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif /* MIR_CLIENT_LTTNG_RPC_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
