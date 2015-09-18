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

#include "session_mediator_report.h"

#include "mir/report/lttng/mir_tracepoint.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "session_mediator_report_tp.h"

#define MIR_SESSION_MEDIATOR_EVENT_METHOD(event)                               \
    void mir::report::lttng::SessionMediatorReport::event(std::string const& app_name) \
    {                                                                          \
        mir_tracepoint(mir_server_session_mediator, event, app_name.c_str());  \
    }

MIR_SESSION_MEDIATOR_EVENT_METHOD(session_connect_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_create_surface_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_next_buffer_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_exchange_buffer_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_submit_buffer_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_allocate_buffers_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_release_buffers_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_release_surface_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_disconnect_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_configure_surface_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_configure_surface_cursor_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_configure_display_called)
MIR_SESSION_MEDIATOR_EVENT_METHOD(session_stop_prompt_session_called)

void mir::report::lttng::SessionMediatorReport::session_start_prompt_session_called(std::string const& app_name, pid_t application_process)
{
    mir_tracepoint(mir_server_session_mediator, session_start_prompt_session_called, app_name.c_str(), application_process);
}

void mir::report::lttng::SessionMediatorReport::session_error(std::string const& app_name, char const* method, std::string const& what)
{
    mir_tracepoint(mir_server_session_mediator, session_error, app_name.c_str(), method, what.c_str());
}

