/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_REPORT_LTTNG_SHARED_LIBRARY_PROBER_REPORT_H_
#define MIR_REPORT_LTTNG_SHARED_LIBRARY_PROBER_REPORT_H_

#include "server_tracepoint_provider.h"
#include "mir/shared_library_prober_report.h"

namespace mir
{
namespace report
{
namespace lttng
{

class SharedLibraryProberReport : public mir::SharedLibraryProberReport
{
public:
    void probing_path(boost::filesystem::path const& path) override;
    void probing_failed(boost::filesystem::path const& path, std::exception const& error) override;
    void loading_library(boost::filesystem::path const& filename) override;
    void loading_failed(boost::filesystem::path const& filename, std::exception const& error) override;

private:
    ServerTracepointProvider tp_provider;
};

}
}
}

#endif /* MIR_REPORT_LTTNG_SHARED_LIBRARY_PROBER_REPORT_H_ */
