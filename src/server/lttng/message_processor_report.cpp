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

#include "mir/lttng/message_processor_report.h"
#include "mir/lttng/mir_tracepoint.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "message_processor_report_tp.h"

void mir::lttng::MessageProcessorReport::received_invocation(
    void const* mediator, int id, std::string const& method)
{
    mir_tracepoint(mir_server_msgproc, received_invocation, mediator, id, method.c_str());
}

void mir::lttng::MessageProcessorReport::completed_invocation(
    void const* mediator, int id, bool result)
{
    mir_tracepoint(mir_server_msgproc, completed_invocation, mediator, id, result);
}

void mir::lttng::MessageProcessorReport::unknown_method(
    void const* /*mediator*/, int /*id*/, std::string const& /*method*/)
{
}

void mir::lttng::MessageProcessorReport::exception_handled(
    void const* /*mediator*/, int /*id*/, std::exception const& /*error*/)
{
}

void mir::lttng::MessageProcessorReport::exception_handled(
    void const* /*mediator*/, std::exception const& /*error*/)
{
}

void mir::lttng::MessageProcessorReport::sent_event(
    void const* /*mediator*/, MirSurfaceEvent const&)
{
    // TODO
}
