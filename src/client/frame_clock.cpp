/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include <cassert>

using mir::client::FrameClock;
using mir::time::PosixTimestamp;

namespace
{
typedef std::unique_lock<std::mutex> Lock;
} // namespace

FrameClock::FrameClock(FrameClock::GetCurrentTime gct)
    : get_current_time{gct}
    , config_changed{false}
    , phase{0}
    , period{0}
    , resync_callback{std::bind(&FrameClock::fallback_resync_callback, this)}
{
}

void FrameClock::set_period(std::chrono::nanoseconds ns)
{
    Lock lock(mutex);
    period = ns;
    config_changed = true;
}

void FrameClock::set_resync_callback(ResyncCallback cb)
{
    Lock lock(mutex);
    resync_callback = cb;
    config_changed = true;
}

PosixTimestamp FrameClock::fallback_resync_callback() const
{
    auto const now = get_current_time(PosixTimestamp().clock_id);
    Lock lock(mutex);
    /*
     * The result here needs to be in phase for all processes that call it,
     * so that nesting servers does not add lag.
     */
    return period != period.zero() ? now - (now % period) : now;
}

PosixTimestamp FrameClock::next_frame_after(PosixTimestamp when) const
{
    Lock lock(mutex);
    /*
     * Unthrottled is an option too. But why?... Because a stream might exist
     * that's not bound to a surface. And if it's not bound to a surface then
     * it has no physical screen to sync to. Hence not throttled at all.
     */
    if (period == period.zero())
        return when;

    /* Phase correction in case 'when' isn't in phase: */
    auto const prev = when - ((when % period) - phase);
    /* Verify it's been corrected: */
    assert(prev % period == phase);

    /*
     * Regardless of render times and scheduling delays, we should always
     * target a perfectly even interval. This results in the greatest
     * visual smoothness as well as providing a catch-up for cases where
     * the client's render time was a little too long.
     */
    auto target = prev + period;

    /*
     * Count missed frames. Note how the target time would need to be more than
     * one frame in the past to count as a missed frame. If the target time
     * is less than one frame in the past then there's still a chance to catch
     * up by rendering the next frame without delay. This avoids decimating
     * to half frame rate.
     *   Compared to triple buffering, this approach is reactive rather than
     * proactive. Both approaches have the same catch-up ability to avoid
     * half frame rate, but this approach has the added benefit of only
     * one frame of lag to the compositor compared to triple buffering's two
     * frames of lag. But we're also doing better than double buffering here
     * in that this approach avoids any additive lag when nested.
     */
    auto now = get_current_time(target.clock_id);
    long const missed_frames = now > target ? (now - target) / period : 0L;

    /*
     * On the first frame and any resumption frame (after the client goes
     * from idle to busy) this will trigger a query to ask for the latest
     * hardware vsync timestamp. So we get in phase with the display.
     * Crucially this is not required on most frames, so that even if it is
     * implemented as a round trip to the server, that won't happen often.
     */
    if (missed_frames > 1 || config_changed)
    {
        lock.unlock();
        auto const server_frame = resync_callback();
        lock.lock();

        phase = server_frame % period;

        /*
         * Avoid mismatches (which will throw) and ensure we're always
         * comparing timestamps of the same clock ID. This means our result
         * 'target' might have a different clock to that of 'prev'. And that's
         * OK... we want to use whatever clock ID the driver is using for
         * its hardware vsync timestamps. We support migrating between clocks.
         */
        now = get_current_time(server_frame.clock_id);

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
             * C++ just guarantees a "signed integer type of at least 64 bits"
             * for std::chrono::nanoseconds::rep
             */
            auto const age_frames = age_ns / period;
            target = server_frame + (age_frames + 1) * period;
        }
        assert(target > now);
        config_changed = false;
    }
    else if (missed_frames > 0)
    {
        /*
         * Low number of missed frames. Try catching up without missing more...
         */
        // FIXME? Missing += operator
        target = target + missed_frames * period;
    }

    return target;
}
