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

#include "periodic_perf_report.h"

using namespace mir::client;

PeriodicPerfReport::PeriodicPerfReport(mir::time::Duration period,
              std::shared_ptr<mir::time::Clock> const& clock)
    : clock(clock)
    , report_interval(period)
    , last_report_time(current_time())
{
}

PeriodicPerfReport::Timestamp PeriodicPerfReport::current_time() const
{
    return clock->now();
}

void PeriodicPerfReport::name_surface(char const* s)
{
    name = s ? s : "?";
}

void PeriodicPerfReport::begin_frame(int buffer_id)
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

void PeriodicPerfReport::end_frame(int buffer_id)
{
    auto now = buffer_end_time[buffer_id] = current_time();
    auto render_time = now - frame_begin_time;
    render_time_sum += render_time;
    ++frame_count;

    auto interval = now - last_report_time;

    if (interval >= report_interval)
    {   // Precision matters. Don't use floats.
        using namespace std::chrono;

        // FPS x 100, remembering to keep millisecond accuracy.
        long fps_100 = frame_count * 100000L /
                       duration_cast<milliseconds>(interval).count();

        auto render_time_avg = render_time_sum / frame_count;
        auto queue_lag_avg = buffer_queue_latency_sum / frame_count;

        // Remove history of old buffer ids
        auto i = buffer_end_time.begin();
        while (i != buffer_end_time.end())
        {
            if ((now - i->second) >= report_interval)
                i = buffer_end_time.erase(i);
            else
                ++i;
        }
        int nbuffers = buffer_end_time.size();

        display(name.c_str(), fps_100,
                duration_cast<microseconds>(render_time_avg).count(),
                duration_cast<microseconds>(queue_lag_avg).count(),
                nbuffers);

        last_report_time = now;
        frame_count = 0;
        render_time_sum = render_time_sum.zero();
        buffer_queue_latency_sum = buffer_queue_latency_sum.zero();
    }
}

