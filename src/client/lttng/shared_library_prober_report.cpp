/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "shared_library_prober_report.h"
#include "mir/report/lttng/mir_tracepoint.h"

#define TRACEPOINT_DEFINE
#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE
#include "shared_library_prober_report_tp.h"

namespace mcl = mir::client;

void mcl::lttng::SharedLibraryProberReport::probing_path(boost::filesystem::path const& path)
{
    mir_tracepoint(mir_client_shared_library_prober, probing_path,
                   path.string().c_str());
}

void mcl::lttng::SharedLibraryProberReport::probing_failed(boost::filesystem::path const& path, std::exception const& error)
{
    mir_tracepoint(mir_client_shared_library_prober, probing_failed,
                   path.string().c_str(), error.what());
}

void mcl::lttng::SharedLibraryProberReport::loading_library(boost::filesystem::path const& filename)
{
    mir_tracepoint(mir_client_shared_library_prober, loading_library,
                   filename.string().c_str());
}

void mcl::lttng::SharedLibraryProberReport::loading_failed(boost::filesystem::path const& filename, std::exception const& error)
{
    mir_tracepoint(mir_client_shared_library_prober, loading_failed,
                   filename.string().c_str(), error.what());
}
