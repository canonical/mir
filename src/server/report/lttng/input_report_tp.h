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
#define TRACEPOINT_PROVIDER mir_server_input

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./input_report_tp.h"

#if !defined(MIR_LTTNG_INPUT_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_INPUT_REPORT_TP_H_

#include "lttng_utils.h"

TRACEPOINT_EVENT(
    mir_server_input,
    received_event_from_kernel,
    TP_ARGS(int64_t, when, int, type, int, code, int, value),
    TP_FIELDS(
        ctf_integer(int64_t, when, when)
        ctf_integer(int, type, type)
        ctf_integer(int, code, code)
        ctf_integer(int, value, value)
     )
)

TRACEPOINT_EVENT(
    mir_server_input,
    published_key_event,
    TP_ARGS(int, dest_fd, uint32_t, seq_id, int64_t, event_time),
    TP_FIELDS(
        ctf_integer(int, dest_fd, dest_fd)
        ctf_integer(uint32_t, seq_id, seq_id)
        ctf_integer(int64_t, event_time, event_time)
     )
)

TRACEPOINT_EVENT(
    mir_server_input,
    published_motion_event,
    TP_ARGS(int, dest_fd, uint32_t, seq_id, int64_t, event_time),
    TP_FIELDS(
        ctf_integer(int, dest_fd, dest_fd)
        ctf_integer(uint32_t, seq_id, seq_id)
        ctf_integer(int64_t, event_time, event_time)
     )
)

TRACEPOINT_EVENT(
    mir_server_input,
    received_event_finished_signal,
    TP_ARGS(int, src_fd, uint32_t, seq_id),
    TP_FIELDS(
        ctf_integer(int, src_fd, src_fd)
        ctf_integer(uint32_t, seq_id, seq_id)
    )
)

TRACEPOINT_EVENT(
    mir_server_input,
    opened_input_device,
    TP_ARGS(const char*, device, const char*, platform),
    TP_FIELDS(
        ctf_string(device, device)
        ctf_string(platform, platform)
    )
)

TRACEPOINT_EVENT(
    mir_server_input,
    failed_to_open_input_device,
    TP_ARGS(const char*, device, const char*, platform),
    TP_FIELDS(
        ctf_string(device, device)
        ctf_string(platform, platform)
    )
)

#endif /* MIR_LTTNG_DISPLAY_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
