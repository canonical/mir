/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER mir_server_connector

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./connector_report_tp.h"

#if !defined(MIR_LTTNG_CONNECTOR_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_CONNECTOR_REPORT_TP_H_

#include "lttng_utils.h"

MIR_LTTNG_VOID_TRACE_CLASS(mir_server_connector)

#define CONNECTOR_TRACE_POINT(name) MIR_LTTNG_VOID_TRACE_POINT(mir_server_connector, name)

CONNECTOR_TRACE_POINT(thread_start)
CONNECTOR_TRACE_POINT(thread_end)
CONNECTOR_TRACE_POINT(scheduled)

#undef CONNECTOR_TRACE_POINT

TRACEPOINT_EVENT(TRACEPOINT_PROVIDER, starting_threads, TP_ARGS(int, count), TP_FIELDS(ctf_integer(int, count, count)))

TRACEPOINT_EVENT(TRACEPOINT_PROVIDER, stopping_threads, TP_ARGS(int, count), TP_FIELDS(ctf_integer(int, count, count)))

TRACEPOINT_EVENT(TRACEPOINT_PROVIDER,
                 creating_session_for,
                 TP_ARGS(int, socket),
                 TP_FIELDS(ctf_integer(int, socket, socket)))

TRACEPOINT_EVENT(TRACEPOINT_PROVIDER,
                 creating_socket_pair,
                 TP_ARGS(int, server, int, client),
                 TP_FIELDS(ctf_integer(int, server, server) ctf_integer(int, client, client)))

TRACEPOINT_EVENT(TRACEPOINT_PROVIDER,
                 listening_on,
                 TP_ARGS(char const*, endpoint),
                 TP_FIELDS(ctf_string(endpoint, endpoint)))

TRACEPOINT_EVENT(TRACEPOINT_PROVIDER,
                 error,
                 TP_ARGS(char const*, diagnostics),
                 TP_FIELDS(ctf_string(diagnostics, diagnostics)))

#include "lttng_utils_pop.h"

#endif /* MIR_LTTNG_CONNECTOR_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
