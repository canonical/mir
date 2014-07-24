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
      last_report_time(current_time()),
      frame_begin_time(0),
      render_time_sum(0),
      frame_count(0)
{
}

void PerfReport::begin_frame()
{
    frame_begin_time = current_time();
}

void PerfReport::end_frame()
{
    auto now = current_time();
    auto render_time = now - frame_begin_time;

    render_time_sum += render_time;
    ++frame_count;

    nsecs_t interval = now - last_report_time;
    if (interval >= report_interval)
    {
        char msg[128];

        // Precision matters. Don't use floats.
        long fps_1000 = frame_count * 1000000L / (interval / 1000000L);
        long render_avg_ms_1000 = render_time_sum / (frame_count * 1000L);

        snprintf(msg, sizeof msg,
                 "%ld.%03ld FPS, render time %ld.%03ldms",
                 fps_1000 / 1000L, fps_1000 % 1000L,
                 render_avg_ms_1000 / 1000L, render_avg_ms_1000 % 1000L
                 );
        logger->log(mir::logging::Logger::informational, msg, component);

        last_report_time = now;
        frame_count = 0;
        render_time_sum = 0;
    }
}

void PerfReport::event_received(MirEvent const&)
{
}

