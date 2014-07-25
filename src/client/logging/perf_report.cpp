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

#define MILLISECONDS(n) ((n) * 1000000LL)
#define SECONDS(n) ((n) * 1000000000LL)

namespace
{

// Use the Android clock. We have to in order to make sense of motion event
// times.
extern "C" nsecs_t systemTime(int clock = 0);
nsecs_t current_time()
{
    return systemTime();
}

nsecs_t const report_interval = SECONDS(1);
const char * const component = "perf"; // Note context is already within client

} // namespace

PerfReport::PerfReport(std::shared_ptr<mir::logging::Logger> const& logger)
    : logger(logger),
      last_report_time(current_time())
{
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
        auto buffer_queue_latency = frame_begin_time -
                                    buffer_end_time[buffer_id];
        buffer_queue_latency_sum += buffer_queue_latency;
    }
}

void PerfReport::end_frame(int buffer_id)
{
    auto now = buffer_end_time[buffer_id] = current_time();
    auto render_time = now - frame_begin_time;
    auto input_lag = oldest_motion_event ? now - oldest_motion_event : 0;
    oldest_motion_event = 0;

    render_time_sum += render_time;
    input_lag_sum += input_lag;
    ++frame_count;

    nsecs_t interval = now - last_report_time;
    if (interval >= report_interval)
    {   // Precision matters. Don't use floats.
        long long interval_ms = interval / MILLISECONDS(1);

        // FPS x 100
        long fps_100 = frame_count * 100000L / interval_ms;

        // Input (motion) events per second
        long input_events_hz = motion_count * 1000L / interval_ms;

        // Frames rendered x 1000
        long frame_count_1000 = frame_count * 1000L;

        // Render time average in microseconds
        long render_avg_usec = render_time_sum / frame_count_1000;

        // Input lag average in microseconds (input event -> client swap)
        long input_avg_usec = input_lag_sum / frame_count_1000;

        // Buffer queue lag average in microseconds (client swap -> screen)
        long queue_avg_usec = buffer_queue_latency_sum / frame_count_1000;

        // Total lag in microseconds (input event -> composited on screen)
        long lag_avg_usec = input_avg_usec + queue_avg_usec;

        int nbuffers = buffer_end_time.size();

        char msg[256];
        snprintf(msg, sizeof msg,
                 "%s: %2ld.%02ld FPS, render %2ld.%02ldms, input lag %2ld.%02ldms, buffer lag %2ld.%02ldms, visible input lag %2ld.%02ldms, %3ldev/s, %d buffers",
                 nam.c_str(),
                 fps_100 / 100, fps_100 % 100,
                 render_avg_usec / 1000, (render_avg_usec / 10) % 100,
                 input_avg_usec / 1000, (input_avg_usec / 10) % 100,
                 queue_avg_usec / 1000, (queue_avg_usec / 10) % 100,
                 lag_avg_usec / 1000, (lag_avg_usec / 10) % 100,
                 input_events_hz,
                 nbuffers
                 );
        logger->log(mir::logging::Logger::informational, msg, component);

        last_report_time = now;
        frame_count = 0;
        motion_count = 0;
        render_time_sum = 0;
        input_lag_sum = 0;
        buffer_queue_latency_sum = 0;

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

void PerfReport::event_received(MirEvent const& e)
{
    if (e.type == mir_event_type_motion)
    {
        ++motion_count;
        if (!oldest_motion_event)
            oldest_motion_event = e.motion.event_time;
    }
}

