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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/report/lttng/mir_tracepoint.h"

#include "input_report.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "input_report_tp.h"

void mir::report::lttng::InputReport::received_event_from_kernel(int64_t when, int type, int code, int value)
{
    mir_tracepoint(mir_server_input, received_event_from_kernel, when, type, code, value);
}

void mir::report::lttng::InputReport::published_key_event(int dest_fd, uint32_t seq_id, int64_t event_time)
{
    mir_tracepoint(mir_server_input, published_key_event, dest_fd, seq_id, event_time);
}

void mir::report::lttng::InputReport::published_motion_event(int dest_fd, uint32_t seq_id, int64_t event_time)
{
    mir_tracepoint(mir_server_input, published_motion_event, dest_fd, seq_id, event_time);
}

void mir::report::lttng::InputReport::received_event_finished_signal(int src_fd, uint32_t seq_id)
{
    mir_tracepoint(mir_server_input, received_event_finished_signal, src_fd, seq_id);
}

void mir::report::lttng::InputReport::opened_input_device(char const* name, char const* platform)
{
    mir_tracepoint(mir_server_input, opened_input_device, name, platform);
}

void mir::report::lttng::InputReport::failed_to_open_input_device(char const* name, char const* platform)
{
    mir_tracepoint(mir_server_input, failed_to_open_input_device, name, platform);
}
