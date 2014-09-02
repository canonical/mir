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

#include "perf_report.h"
#include "mir/logging/logger.h"

using namespace mir::client::logging;
using namespace std::chrono;

namespace
{
const char * const component = "perf"; // Note context is already within client
} // namespace

PerfReport::PerfReport(std::shared_ptr<mir::logging::Logger> const& logger)
    : logger(logger),
      last_report_time(current_time())
{
}

PerfReport::Timestamp PerfReport::current_time() const
{
    return high_resolution_clock::now();
}

void PerfReport::name(char const* s)
{
    nam = s ? s : "?";
}

void PerfReport::begin_frame(int buffer_id)
{
    frame_begin_time = current_time();

    if (buffer_end_time.find(buffer_id) != buffer_end_time.end())
    {
        // Estimate page flip (composition finished) time as the time we
        // eventually get the same buffer back.
        auto estimated_page_flip_time = frame_begin_time;
        auto buffer_queue_latency = estimated_page_flip_time -
                                    buffer_end_time[buffer_id];
        buffer_queue_latency_sum += buffer_queue_latency;
    }
}

void PerfReport::end_frame(int buffer_id)
{
    auto now = buffer_end_time[buffer_id] = current_time();

    auto render_time = now - frame_begin_time;
    render_time_sum += render_time;

    ++frame_count;

    Duration interval = now - last_report_time;
    const Duration report_interval = seconds(1);
    if (interval >= report_interval)
    {   // Precision matters. Don't use floats.
        // FPS x 100, remembering to keep millisecond accuracy.
        long fps_100 = frame_count * 100000L /
                       duration_cast<milliseconds>(interval).count();

        // Client render time average in microseconds
        long render_time_avg_usec =
            duration_cast<microseconds>(render_time_sum).count() / frame_count;

        // Buffer queue lag average in microseconds (frame end -> screen)
        long queue_lag_avg_usec =
            duration_cast<microseconds>(buffer_queue_latency_sum).count() /
            frame_count;

        // Visible lag in microseconds (render time + queue lag)
        long visible_lag_avg_usec = render_time_avg_usec + queue_lag_avg_usec;

        int nbuffers = buffer_end_time.size();

        char msg[256];
        snprintf(msg, sizeof msg,
                 "%s: %2ld.%02ld FPS, render %2ld.%02ldms, compositor lag %2ld.%02ldms, visible lag %2ld.%02ldms, %d buffers",
                 nam.c_str(),
                 fps_100 / 100, fps_100 % 100,
                 render_time_avg_usec / 1000, (render_time_avg_usec / 10) % 100,
                 queue_lag_avg_usec / 1000, (queue_lag_avg_usec / 10) % 100,
                 visible_lag_avg_usec / 1000, (visible_lag_avg_usec / 10) % 100,
                 nbuffers
                 );
        logger->log(mir::logging::Logger::informational, msg, component);

        last_report_time = now;
        frame_count = 0;
        render_time_sum = Duration::zero();
        buffer_queue_latency_sum = Duration::zero();

        // Remove history of old buffer ids
        auto i = buffer_end_time.begin();
        while (i != buffer_end_time.end())
        {
            if ((now - i->second) >= report_interval)
                i = buffer_end_time.erase(i);
            else
                ++i;
        }
    }
}

