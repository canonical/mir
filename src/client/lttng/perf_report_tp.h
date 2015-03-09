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

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER mir_client_perf

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./perf_report_tp.h"

#if !defined(MIR_CLIENT_LTTNG_PERF_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_CLIENT_LTTNG_PERF_REPORT_TP_H_

#include <lttng/tracepoint.h>
#include <stdint.h>

TRACEPOINT_EVENT(
    mir_client_perf,
    begin_frame,
    TP_ARGS(uint32_t, buffer_id),
    TP_FIELDS(
        ctf_integer(uint32_t, buffer_id, buffer_id)
    )
)

TRACEPOINT_EVENT(
    mir_client_perf,
    end_frame,
    TP_ARGS(uint32_t, buffer_id),
    TP_FIELDS(
        ctf_integer(uint32_t, buffer_id, buffer_id)
    )
)

#endif /* MIR_CLIENT_LTTNG_PERF_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
