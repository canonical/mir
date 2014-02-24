/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER mir_client_input_receiver

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./input_receiver_report_tp.h"

#if !defined(MIR_CLIENT_LTTNG_INPUT_RECEIVER_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_CLIENT_LTTNG_INPUT_RECEIVER_REPORT_TP_H_

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
    mir_client_input_receiver,
    key_event,
    TP_ARGS(int32_t, device_id, int32_t, source_id, int, action, int, flags, unsigned int, modifiers, int32_t, key_code, int32_t, scan_code, int64_t, down_time, int64_t, event_time),
    TP_FIELDS(
        ctf_integer(int32_t, device_id, device_id)
        ctf_integer(int32_t, source_id, source_id)
        ctf_integer(int, action, action)
        ctf_integer(int, flags, flags)
        ctf_integer(unsigned int, modifiers, modifiers)
        ctf_integer(int32_t, key_code, key_code)
        ctf_integer(int32_t, scan_code, scan_code)
        ctf_integer(int64_t, down_time, down_time)
        ctf_integer(int64_t, event_time, event_time)
    )
)

TRACEPOINT_EVENT(
    mir_client_input_receiver,
    motion_event,
    TP_ARGS(int32_t, device_id, int32_t, source_id, int, action, int, flags, unsigned int, modifiers, int32_t, edge_flags, int, button_state, int64_t, down_time, int64_t, event_time),
    TP_FIELDS(
        ctf_integer(int32_t, device_id, device_id)
        ctf_integer(int32_t, source_id, source_id)
        ctf_integer(int, action, action)
        ctf_integer(int, flags, flags)
        ctf_integer(unsigned int, modifiers, modifiers)
        ctf_integer(int32_t, edge_flags, edge_flags)
        ctf_integer(int, button_state, button_state)
        ctf_integer(int64_t, down_time, down_time)
        ctf_integer(int64_t, event_time, event_time)
    )
)

TRACEPOINT_EVENT(
    mir_client_input_receiver,
    motion_event_coordinate,
    TP_ARGS(int, id, float, x, float, y, float, touch_major, float, touch_minor, float, size, float, pressure, float, orientation),
    TP_FIELDS(
        ctf_integer(int, id, id)
        ctf_float(float, x, x)
        ctf_float(float, y, y)
        ctf_float(float, touch_major, touch_major)
        ctf_float(float, touch_minor, touch_minor)
        ctf_float(float, size, size)
        ctf_float(float, pressure, pressure)
        ctf_float(float, orientation, orientation)
    )
)

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif /* MIR_CLIENT_LTTNG_INPUT_RECEIVER_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
