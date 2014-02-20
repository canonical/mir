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

#include "scene_report.h"
#include "mir/report/lttng/mir_tracepoint.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "scene_report_tp.h"

void mir::report::lttng::SceneReport::surface_created(BasicSurfaceId, std::string const& name)
{
    mir_tracepoint(mir_server_scene, surface_created, name.c_str());
}

void mir::report::lttng::SceneReport::surface_added(BasicSurfaceId, std::string const& name)
{
    mir_tracepoint(mir_server_scene, surface_added, name.c_str());
}

void mir::report::lttng::SceneReport::surface_removed(BasicSurfaceId, std::string const& name)
{
    mir_tracepoint(mir_server_scene, surface_removed, name.c_str());
}

void mir::report::lttng::SceneReport::surface_deleted(BasicSurfaceId, std::string const& name)
{
    mir_tracepoint(mir_server_scene, surface_deleted, name.c_str());
}
