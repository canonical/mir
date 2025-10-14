/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "shared_library_prober_report.h"

#include "mir/report/lttng/mir_tracepoint.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "shared_library_prober_report_tp.h"

namespace mrl = mir::report::lttng;

void mrl::SharedLibraryProberReport::probing_path(std::filesystem::path const& path)
{
    mir_tracepoint(mir_server_shared_library_prober, probing_path,
                   path.string().c_str());
}

void mrl::SharedLibraryProberReport::probing_failed(std::filesystem::path const& path, std::exception const& error)
{
    mir_tracepoint(mir_server_shared_library_prober, probing_failed,
                   path.string().c_str(), error.what());
}

void mrl::SharedLibraryProberReport::loading_library(std::filesystem::path const& filename)
{
    mir_tracepoint(mir_server_shared_library_prober, loading_library,
                   filename.string().c_str());
}

void mrl::SharedLibraryProberReport::loading_failed(std::filesystem::path const& filename, std::exception const& error)
{
    mir_tracepoint(mir_server_shared_library_prober, loading_failed,
                   filename.string().c_str(), error.what());
}
