/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_CLIENT_LOGGING_PERF_REPORT_H_
#define MIR_CLIENT_LOGGING_PERF_REPORT_H_

#include "../periodic_perf_report.h"
#include <memory>

namespace mir
{
namespace logging { class Logger; }

namespace client
{
namespace logging
{

class PerfReport : public PeriodicPerfReport
{
public:
    PerfReport(std::shared_ptr<mir::logging::Logger> const& logger);
    void display(const char *name, long fps100, long rendertime_usec,
                 long lag_usec, int nbuffers) const override;
private:
    std::shared_ptr<mir::logging::Logger> const logger;
};

} // namespace logging
} // namespace client
} // namespace mir

#endif // MIR_CLIENT_LOGGING_PERF_REPORT_H_
