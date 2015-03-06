/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "perf_report.h"
#include "mir/report/lttng/mir_tracepoint.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "perf_report_tp.h"

namespace mcl = mir::client;

void mcl::lttng::PerfReport::name_surface(char const*)
{
}

void mcl::lttng::PerfReport::begin_frame(int buffer_id)
{
    mir_tracepoint(mir_client_perf, begin_frame, buffer_id);
}

void mcl::lttng::PerfReport::end_frame(int buffer_id)
{
    mir_tracepoint(mir_client_perf, end_frame, buffer_id);
}
