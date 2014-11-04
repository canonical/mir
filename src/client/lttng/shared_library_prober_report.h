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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_CLIENT_LTTNG_SHARED_LIBRARY_PROBER_REPORT_H_
#define MIR_CLIENT_LTTNG_SHARED_LIBRARY_PROBER_REPORT_H_

#include "mir/shared_library_prober_report.h"
#include "client_tracepoint_provider.h"

namespace mir
{
namespace client
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
    ClientTracepointProvider tp_provider;
};

}
}
}

#endif /* MIR_CLIENT_LTTNG_SHARED_LIBRARY_PROBER_REPORT_H_ */


#ifndef SHARED_LIBRARY_PROBER_REPORT_H
#define SHARED_LIBRARY_PROBER_REPORT_H


#endif // SHARED_LIBRARY_PROBER_REPORT_H
