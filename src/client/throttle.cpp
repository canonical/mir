/*
 * Copyright © 2016 Canonical Ltd.
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

#include "throttle.h"
#include <stdexcept>

using namespace mir;
using namespace mir::time;

Throttle::Throttle(Throttle::GetCurrentTime gct)
    : get_current_time{gct}, readjustment_required{false}
{
    set_frequency(60);
    set_resync_callback(std::bind(&Throttle::fake_resync_callback, this));
}

void Throttle::set_period(std::chrono::nanoseconds ns)
{
    period = ns;
    readjustment_required = true;
}

void Throttle::set_frequency(double hz)
{
    if (hz <= 0.0)
        throw std::logic_error("Throttle::set_frequency is not positive");
    set_period(std::chrono::nanoseconds(static_cast<long>(1000000000L / hz)));
}

void Throttle::set_resync_callback(ResyncCallback cb)
{
    resync_callback = cb;
    readjustment_required = true;
}

PosixTimestamp Throttle::fake_resync_callback() const
{
    fprintf(stderr, "fake_resync_callback\n");
    auto const now = get_current_time(CLOCK_MONOTONIC);
    return now - (now % period);
}

PosixTimestamp Throttle::next_frame_after(PosixTimestamp prev) const
{
    /*
     * Regardless of render times and scheduling delays, we should always
     * target a perfectly even interval. This results in the greatest
     * visual smoothness as well as providing a catch-up for cases where
     * the client's render time was a little too long.
     */
    auto target = prev + period;

    /*
     * On the first frame, and any resumption frame (after the client goes
     * from idle to busy) this will trigger a query to the server (or virtual
     * server) to ask for the latest hardware vsync timestamp. So we get
     * in perfect sync with the display, but only need to query the server
     * like this occasionally and not on every frame...
     */
    auto const now = get_current_time(target.clock_id);
    if (target < now || readjustment_required)
    {
        auto const server_frame = resync_callback();
        if (server_frame > now)
        {
            target = server_frame;
        }
        else
        {
            auto const age_ns = now - server_frame;
            // Ensure age_frames gets truncated if not already.
            // C++ just guarantees "signed integer type of at least 64 bits"
            // for std::chrono::nanoseconds::rep
            auto const age_frames = age_ns / period;
            target = server_frame + (age_frames + 1) * period;
        }
        readjustment_required = false;
    }

#if 1
    auto delta = target - prev;
    long usec = delta.count() / 1000;
    fprintf(stderr, "Wait delta %ld.%03ldms\n", usec/1000, usec%1000);
#endif

    return target;
}
