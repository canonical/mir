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
#define TRACEPOINT_PROVIDER mir_server_display

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./display_report_tp.h"

#if !defined(MIR_LTTNG_DISPLAY_REPORT_TP_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define MIR_LTTNG_DISPLAY_REPORT_TP_H_

#include "lttng_utils.h"


MIR_LTTNG_VOID_TRACE_CLASS(mir_server_display)

#define DISPLAY_REPORT_TRACE_POINT(name) MIR_LTTNG_VOID_TRACE_POINT(mir_server_display,name)

DISPLAY_REPORT_TRACE_POINT(report_successful_setup_of_native_resources)
DISPLAY_REPORT_TRACE_POINT(report_successful_egl_make_current_on_construction)
DISPLAY_REPORT_TRACE_POINT(report_successful_egl_buffer_swap_on_construction)
DISPLAY_REPORT_TRACE_POINT(report_successful_display_construction)
DISPLAY_REPORT_TRACE_POINT(report_successful_drm_mode_set_crtc_on_construction)
DISPLAY_REPORT_TRACE_POINT(report_vt_switch_away_failure)
DISPLAY_REPORT_TRACE_POINT(report_vt_switch_back_failure)
DISPLAY_REPORT_TRACE_POINT(report_gpu_composition_in_use)

#undef DISPLAY_REPORT_TRACE_POINT

TRACEPOINT_EVENT(
    mir_server_display,
    report_drm_master_failure,
    TP_ARGS(char const*, error_string),
    TP_FIELDS(
        ctf_string(error_string, error_string)
     )
)

TRACEPOINT_EVENT(
    mir_server_display,
    report_hwc_composition_in_use,
    TP_ARGS(int, major, int, minor),
    TP_FIELDS(
        ctf_integer(int, major, major)
        ctf_integer(int, minor, minor)
    )
)

TRACEPOINT_EVENT(
    mir_server_display,
    report_vsync,
    TP_ARGS(int, id),
    TP_FIELDS(
        ctf_integer(int, id, id)
     )
)

#endif /* MIR_LTTNG_DISPLAY_REPORT_TP_H_ */

#include <lttng/tracepoint-event.h>
