/*
 * Copyright © 2013-2014 Canonical Ltd.
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

using namespace mir::time;
namespace ml = mir::logging;
namespace mrl = mir::report::logging;

namespace
{
    const char * const component = "compositor";
    const auto min_report_interval = std::chrono::seconds(1);
}

mrl::CompositorReport::CompositorReport(
    std::shared_ptr<ml::Logger> const& logger,
    std::shared_ptr<Clock> const& clock)
    : logger(logger),
      clock(clock),
      last_report(now())
{
}

mrl::CompositorReport::TimePoint mrl::CompositorReport::now() const
{
    return clock->now();
}

void mrl::CompositorReport::added_display(int width, int height, int x, int y, SubCompositorId id)
{
    char msg[128];
    snprintf(msg, sizeof msg, "Added display %p: %dx%d %+d%+d",
             id, width, height, x, y);
    logger->log(ml::Severity::informational, msg, component);
}

void mrl::CompositorReport::began_frame(SubCompositorId id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];

    auto t = now();
    inst.start_of_frame = t;
    inst.latency_sum += t - last_scheduled;
    inst.bypassed = true;
}

void mrl::CompositorReport::renderables_in_frame(SubCompositorId, mir::graphics::RenderableList const&)
{
}

void mrl::CompositorReport::rendered_frame(SubCompositorId id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];
    inst.render_time_sum += now() - inst.start_of_frame;
    inst.bypassed = false;
}

void mrl::CompositorReport::Instance::log(ml::Logger& logger, SubCompositorId id)
{
    // The first report is a valid sample, but don't log anything because
    // we need at least two samples for valid deltas.
    if (last_reported_total_time_sum > TimePoint())
    {
        long long dt =
            std::chrono::duration_cast<std::chrono::microseconds>(
                total_time_sum - last_reported_total_time_sum
            ).count();
        auto dn = nframes - last_reported_nframes;
        long long dr =
            std::chrono::duration_cast<std::chrono::microseconds>(
                render_time_sum - last_reported_render_time_sum
            ).count();
        long long dl =
            std::chrono::duration_cast<std::chrono::microseconds>(
                latency_sum - last_reported_latency_sum
            ).count();

        long bypass_percent = (nbypassed - last_reported_bypassed) * 100L / dn;

        // Keep everything premultiplied by 1000 to guarantee accuracy
        // and avoid floating point.
        long frames_per_1000sec = dt ? dn * 1000000000LL / dt : 0;
        long avg_render_time_usec = dn ? dr / dn : 0;
        long avg_latency_usec = dn ? dl / dn : 0;
        long dt_msec = dt / 1000L;

        char msg[128];
        snprintf(msg, sizeof msg, "Display %p averaged %ld.%03ld FPS, "
                 "%ld.%03ld ms/frame, "
                 "latency %ld.%03ld ms, "
                 "%ld frames over %ld.%03ld sec, "
                 "%ld%% bypassed",
                 id,
                 frames_per_1000sec / 1000,
                 frames_per_1000sec % 1000,
                 avg_render_time_usec / 1000,
                 avg_render_time_usec % 1000,
                 avg_latency_usec / 1000,
                 avg_latency_usec % 1000,
                 dn,
                 dt_msec / 1000,
                 dt_msec % 1000,
                 bypass_percent
                 );

        logger.log(ml::Severity::informational, msg, component);
    }

    last_reported_total_time_sum = total_time_sum;
    last_reported_render_time_sum = render_time_sum;
    last_reported_latency_sum = latency_sum;
    last_reported_nframes = nframes;
    last_reported_bypassed = nbypassed;
}

void mrl::CompositorReport::finished_frame(SubCompositorId id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];

    auto t = now();
    inst.total_time_sum += t - inst.end_of_frame;
    inst.end_of_frame = t;
    inst.nframes++;
    if (inst.bypassed)
        ++inst.nbypassed;

    /*
     * The exact reporting interval doesn't matter because we count everything
     * as a Reimann sum. Results will simply be the average over the interval.
     */
    if ((t - last_report) >= min_report_interval)
    {
        last_report = t;

        for (auto& i : instance)
            i.second.log(*logger, i.first);
    }

    if (inst.bypassed != inst.prev_bypassed || inst.nframes == 1)
    {
        char msg[128];
        snprintf(msg, sizeof msg, "Display %p bypass %s",
                 id, inst.bypassed ? "ON" : "OFF");
        logger->log(ml::Severity::informational, msg, component);
    }
    inst.prev_bypassed = inst.bypassed;
}

void mrl::CompositorReport::started()
{
    logger->log(ml::Severity::informational, "Started", component);
}

void mrl::CompositorReport::stopped()
{
    logger->log(ml::Severity::informational, "Stopped", component);

    std::lock_guard<std::mutex> lock(mutex);
    instance.clear();
}

void mrl::CompositorReport::scheduled()
{
    std::lock_guard<std::mutex> lock(mutex);
    last_scheduled = now();
}
