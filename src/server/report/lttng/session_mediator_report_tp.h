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
#define TRACEPOINT_PROVIDER mir_server_session_mediator

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./session_mediator_report_tp.h"

#if !defined(MIR_LTTNG_SESSION_MEDIATOR_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_SESSION_MEDIATOR_REPORT_TP_H_

#include "lttng_utils.h"

TRACEPOINT_EVENT_CLASS(
    mir_server_session_mediator,
    application_event,
    TP_ARGS(char const*, application),
    TP_FIELDS(
        ctf_string(application, application)
        )
    )

#define MIR_SESSION_MEDIATOR_EVENT(event) \
    TRACEPOINT_EVENT_INSTANCE(mir_server_session_mediator, application_event, event, TP_ARGS(char const*, application))

MIR_SESSION_MEDIATOR_EVENT(session_connect_called)
MIR_SESSION_MEDIATOR_EVENT(session_create_surface_called)
MIR_SESSION_MEDIATOR_EVENT(session_next_buffer_called)
MIR_SESSION_MEDIATOR_EVENT(session_exchange_buffer_called)
MIR_SESSION_MEDIATOR_EVENT(session_submit_buffer_called)
MIR_SESSION_MEDIATOR_EVENT(session_allocate_buffers_called)
MIR_SESSION_MEDIATOR_EVENT(session_release_buffers_called)
MIR_SESSION_MEDIATOR_EVENT(session_release_surface_called)
MIR_SESSION_MEDIATOR_EVENT(session_disconnect_called)
MIR_SESSION_MEDIATOR_EVENT(session_configure_surface_called)
MIR_SESSION_MEDIATOR_EVENT(session_configure_surface_cursor_called)
MIR_SESSION_MEDIATOR_EVENT(session_configure_display_called)
MIR_SESSION_MEDIATOR_EVENT(session_stop_prompt_session_called)

TRACEPOINT_EVENT(
    mir_server_session_mediator,
    session_start_prompt_session_called,
    TP_ARGS(char const*, application, pid_t, application_process),
    TP_FIELDS(
        ctf_string(application, application)
        ctf_integer(pid_t, application_process, application_process)
        )
    )

TRACEPOINT_EVENT(
    mir_server_session_mediator,
    session_error,
    TP_ARGS(char const*, application, char const*, method, char const*, what),
    TP_FIELDS(
        ctf_string(application, application)
        ctf_string(method, method)
        ctf_string(what, what)
        )
    )

#undef MIR_SESSION_MEDIATOR_EVENT

#endif /* MIR_LTTNG_SESSION_MEDIATOR_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
