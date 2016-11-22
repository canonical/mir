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

#include "frame_clock.h"
#include <stdexcept>

using namespace mir;
using namespace mir::time;

FrameClock::FrameClock(FrameClock::GetCurrentTime gct)
    : get_current_time{gct}
    , resync_required{false}
    , fallback_resync_clock{CLOCK_MONOTONIC}
    , period{0}
    , resync_callback{std::bind(&FrameClock::fallback_resync_callback, this)}
{
}

void FrameClock::set_period(std::chrono::nanoseconds ns)
{
    period = ns;
    resync_required = true;
}

void FrameClock::set_resync_callback(ResyncCallback cb)
{
    resync_callback = cb;
    resync_required = true;
}

PosixTimestamp FrameClock::fallback_resync_callback() const
{
    fprintf(stderr, "fallback_resync_callback\n");
    return get_current_time(fallback_resync_clock);
}

PosixTimestamp FrameClock::next_frame_after(PosixTimestamp prev) const
{
    /*
     * Unthrottled is an option too. But why?... Because a stream might exist
     * that's not bound to a surface. And if it's not bound to a surface then
     * it has no physical screen to sync to. Hence not throttled at all.
     */
    if (period == period.zero())
        return prev;

    /*
     * Regardless of render times and scheduling delays, we should always
     * target a perfectly even interval. This results in the greatest
     * visual smoothness as well as providing a catch-up for cases where
     * the client's render time was a little too long.
     */
    auto target = prev + period;
    auto const now = get_current_time(target.clock_id);

    /*
     * On the first frame and any resumption frame (after the client goes
     * from idle to busy) this will trigger a query to the server (or virtual
     * server) to ask for the latest hardware vsync timestamp. So we get
     * in phase with the display. Note we only need to query the server
     * like this occasionally and not on every frame, which is good for IPC.
     */
    if (target < now || resync_required)
    {
        fallback_resync_clock = prev.clock_id; // If required at all
        auto const server_frame = resync_callback();
        /*
         * It's important to target a future time and not allow 'now'. This
         * is because we will have some buffer queues for the time being that
         * have swapinterval 1, which if overfilled by being too agressive will
         * cause visual lag. After all buffer queues move to always dropping
         * (mailbox mode), this delay won't be required as a safety.
         */
        if (server_frame > now)
        {
            target = server_frame;
        }
        else
        {
            auto const age_ns = now - server_frame;
            /*
             * Ensure age_frames gets truncated if not already.
             * C++ just guarantees "signed integer type of at least 64 bits"
             * for std::chrono::nanoseconds::rep
             */
            auto const age_frames = age_ns / period;
            target = server_frame + (age_frames + 1) * period;
        }
        resync_required = false;
    }

#if 1
    auto delta = target - prev;
    long usec = delta.count() / 1000;
    fprintf(stderr, "Wait delta %ld.%03ldms\n", usec/1000, usec%1000);
#endif

    return target;
}
