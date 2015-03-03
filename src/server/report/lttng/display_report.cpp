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

#include "display_report.h"

#include "mir/report/lttng/mir_tracepoint.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "display_report_tp.h"

#include "lttng_utils.h"

#define DISPLAY_REPORT_TRACE_CALL(name) MIR_LTTNG_VOID_TRACE_CALL(DisplayReport,mir_server_display,name)

DISPLAY_REPORT_TRACE_CALL(report_successful_setup_of_native_resources)
DISPLAY_REPORT_TRACE_CALL(report_successful_egl_make_current_on_construction)
DISPLAY_REPORT_TRACE_CALL(report_successful_egl_buffer_swap_on_construction)
DISPLAY_REPORT_TRACE_CALL(report_successful_display_construction)
DISPLAY_REPORT_TRACE_CALL(report_successful_drm_mode_set_crtc_on_construction)
DISPLAY_REPORT_TRACE_CALL(report_vt_switch_away_failure)
DISPLAY_REPORT_TRACE_CALL(report_vt_switch_back_failure)

#undef DISPLAY_REPORT_TRACE_CALL

void mir::report::lttng::DisplayReport::report_egl_configuration(EGLDisplay /*disp*/, EGLConfig /*config*/)
{
}

void mir::report::lttng::DisplayReport::report_drm_master_failure(int error)
{
    mir_tracepoint(mir_server_display, report_drm_master_failure, strerror(error));
}

void mir::report::lttng::DisplayReport::report_vsync(unsigned int display_id)
{
    mir_tracepoint(mir_server_display, report_vsync, display_id);
}
