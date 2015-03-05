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

#include "compositor_report.h"

#include "mir/graphics/buffer.h"
#include "mir/report/lttng/mir_tracepoint.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "compositor_report_tp.h"

#define COMPOSITOR_TRACE_CALL(name) MIR_LTTNG_VOID_TRACE_CALL(CompositorReport, mir_server_compositor, name)

COMPOSITOR_TRACE_CALL(started)
COMPOSITOR_TRACE_CALL(stopped)
COMPOSITOR_TRACE_CALL(scheduled)

#undef COMPOSITOR_TRACE_CALL

void mir::report::lttng::CompositorReport::added_display(int width, int height, int x, int y, SubCompositorId id)
{
    mir_tracepoint(mir_server_compositor, added_display, width, height, x, y, id);
}

void mir::report::lttng::CompositorReport::began_frame(SubCompositorId id)
{
    mir_tracepoint(mir_server_compositor, began_frame, id);
}

void mir::report::lttng::CompositorReport::renderables_in_frame(
    SubCompositorId id, graphics::RenderableList const& list)
{
    std::vector<uint32_t> ids(list.size());
    auto it = list.begin();
    for(auto& id : ids)
        id = (*it++)->buffer()->id().as_value();
    mir_tracepoint(mir_server_compositor, buffers_in_frame, id, ids.data(), ids.size());
}

void mir::report::lttng::CompositorReport::rendered_frame(SubCompositorId id)
{
    mir_tracepoint(mir_server_compositor, rendered_frame, id);
}

void mir::report::lttng::CompositorReport::finished_frame(SubCompositorId id)
{
    mir_tracepoint(mir_server_compositor, finished_frame, id);
}
