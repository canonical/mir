/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include "../perf_report.h"
#include "mir_toolkit/event.h"
#include <memory>

namespace mir
{
namespace logging { class Logger; }

namespace client
{
namespace logging
{

class PerfReport : public mir::client::PerfReport
{
public:
    PerfReport(std::shared_ptr<mir::logging::Logger> const& logger);
    void begin_frame() override;
    void end_frame() override;
    void event_received(MirEvent const&) override;
private:
    std::shared_ptr<mir::logging::Logger> const logger;
    nsecs_t last_report_time;
    nsecs_t frame_begin_time;
    nsecs_t render_time_sum;
    int frame_count;
};

} // namespace logging
} // namespace client
} // namespace mir

#endif // MIR_CLIENT_LOGGING_PERF_REPORT_H_
