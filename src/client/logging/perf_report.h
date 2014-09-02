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
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace mir
{
namespace logging { class Logger; }

namespace client
{
namespace logging
{

class PeriodicPerfReport : public mir::client::PerfReport
{
public:
    typedef std::chrono::high_resolution_clock::duration Duration;

    PeriodicPerfReport(Duration period);
    void name_surface(char const*) override;
    void begin_frame(int buffer_id) override;
    void end_frame(int buffer_id) override;
    virtual void display(const char *name, long fps100,
                         Duration rendertime, Duration lag,
                         int nbuffers) const = 0;
private:
    typedef std::chrono::high_resolution_clock::time_point Timestamp;
    Timestamp current_time() const;
    Duration const report_interval;
    std::string name;
    Timestamp last_report_time;
    Timestamp frame_begin_time;
    Timestamp frame_end_time;
    Duration render_time_sum = Duration::zero();
    Duration buffer_queue_latency_sum = Duration::zero();
    int frame_count = 0;
    std::unordered_map<int,Timestamp> buffer_end_time;
};

class PerfReport : public PeriodicPerfReport
{
public:
    PerfReport(std::shared_ptr<mir::logging::Logger> const& logger);
    void display(const char *name, long fps100, Duration rendertime,
                 Duration lag, int nbuffers) const override;
private:
    std::shared_ptr<mir::logging::Logger> const logger;
};

} // namespace logging
} // namespace client
} // namespace mir

#endif // MIR_CLIENT_LOGGING_PERF_REPORT_H_
