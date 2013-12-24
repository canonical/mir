/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "compositor_report.h"
#include "mir/logging/logger.h"

using namespace mir;
using namespace mir::compositor;
using namespace mir::time;

namespace
{
    const char * const component = "compositor";
    const auto min_report_interval = std::chrono::seconds(1);
}

logging::CompositorReport::CompositorReport(
    std::shared_ptr<Logger> const& logger,
    std::shared_ptr<TimeSource> const& clock)
    : logger(logger),
      clock(clock),
      last_report(now())
{
}

logging::CompositorReport::TimePoint logging::CompositorReport::now() const
{
    return clock->sample();
}

void logging::CompositorReport::added_display(int width, int height,
                                              int x, int y,
                                              SubCompositorId id)
{
    char msg[128];
    snprintf(msg, sizeof msg, "Added display %p: %dx%d %+d%+d",
             id, width, height, x, y);
    logger->log(Logger::informational, msg, component);
}

void logging::CompositorReport::began_frame(SubCompositorId id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];

    auto t = now();
    inst.start_of_frame = t;
    inst.latency_sum += t - last_scheduled;
}

void logging::CompositorReport::finished_frame(SubCompositorId id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];

    auto t = now();
    inst.total_time_sum += t - inst.end_of_frame;
    inst.frame_time_sum += t - inst.start_of_frame;
    inst.end_of_frame = t;
    inst.nframes++;

    /*
     * The exact reporting interval doesn't matter because we count everything
     * as a Reimann sum. Results will simply be the average over the interval.
     */
    if ((t - last_report) >= min_report_interval)
    {
        last_report = t;

        for (auto& ip : instance)
        {
            auto &i = ip.second;

            if (i.last_reported_total_time_sum > TimePoint())
            {
                long long dt =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        i.total_time_sum - i.last_reported_total_time_sum
                    ).count();
                auto dn = i.nframes - i.last_reported_nframes;
                long long df =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        i.frame_time_sum - i.last_reported_frame_time_sum
                    ).count();
                long long dl =
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        i.latency_sum - i.last_reported_latency_sum
                    ).count();

                // Keep everything premultiplied by 1000 to guarantee accuracy
                // and avoid floating point.
                long frames_per_1000sec = dt ? dn * 1000000000LL / dt : 0;
                long avg_frame_time_usec = dn ? df / dn : 0;
                long avg_latency_usec = dn ? dl / dn : 0;
                long dt_msec = dt / 1000L;

                char msg[128];
                snprintf(msg, sizeof msg, "Display %p averaged %ld.%03ld FPS, "
                         "%ld.%03ld ms/frame, "
                         "latency %ld.%03ld ms, "
                         "%ld frames over %ld.%03ld sec",
                         ip.first,
                         frames_per_1000sec / 1000,
                         frames_per_1000sec % 1000,
                         avg_frame_time_usec / 1000,
                         avg_frame_time_usec % 1000,
                         avg_latency_usec / 1000,
                         avg_latency_usec % 1000,
                         dn,
                         dt_msec / 1000,
                         dt_msec % 1000
                         );

                logger->log(Logger::informational, msg, component);
            }

            i.last_reported_total_time_sum = i.total_time_sum;
            i.last_reported_frame_time_sum = i.frame_time_sum;
            i.last_reported_latency_sum = i.latency_sum;
            i.last_reported_nframes = i.nframes;
        }
    }
}

void logging::CompositorReport::started()
{
    logger->log(Logger::informational, "Started", component);
}

void logging::CompositorReport::stopped()
{
    logger->log(Logger::informational, "Stopped", component);
}

void logging::CompositorReport::scheduled()
{
    std::lock_guard<std::mutex> lock(mutex);
    last_scheduled = now();
}
