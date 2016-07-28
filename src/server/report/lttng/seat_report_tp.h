/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Brandon Schaefer <brandon.schaefer@canonical.com>
 */

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER mir_server_seat

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./seat_report_tp.h"

#if !defined(MIR_LTTNG_SEAT_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_SEAT_REPORT_TP_H_

#include "lttng_utils.h"

TRACEPOINT_EVENT(
    mir_server_seat,
    seat_set_confinement_region_called,
    TP_ARGS(int, x, int, y, int, width, int, height),
    TP_FIELDS(
        ctf_integer(int, x, x)
        ctf_integer(int, y, y)
        ctf_integer(int, width, width)
        ctf_integer(int, height, height)
     )
)

#endif /* MIR_LTTNG_SEAT_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
