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

#include "connector_report.h"

#include "mir/report/lttng/mir_tracepoint.h"

#include <boost/exception/diagnostic_information.hpp>

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "connector_report_tp.h"

#define CONNECTOR_TRACE_CALL(name) MIR_LTTNG_VOID_TRACE_CALL(ConnectorReport, mir_server_connector, name)

CONNECTOR_TRACE_CALL(thread_start)
CONNECTOR_TRACE_CALL(thread_end)

#undef CONNECTOR_TRACE_CALL

void mir::report::lttng::ConnectorReport::creating_session_for(int socket_handle)
{
    mir_tracepoint(mir_server_connector, creating_session_for, socket_handle);
}

void mir::report::lttng::ConnectorReport::creating_socket_pair(int server_handle, int client_handle)
{
    mir_tracepoint(mir_server_connector, creating_socket_pair, server_handle, client_handle);
}

void mir::report::lttng::ConnectorReport::listening_on(std::string const& endpoint)
{
    mir_tracepoint(mir_server_connector, listening_on, endpoint.c_str());
}

void mir::report::lttng::ConnectorReport::error(std::exception const& error)
{
    mir_tracepoint(mir_server_connector, error, boost::diagnostic_information(error).c_str());
}

void mir::report::lttng::ConnectorReport::warning(std::string const& error)
{
    mir_tracepoint(mir_server_connector, error, error.c_str());
}

