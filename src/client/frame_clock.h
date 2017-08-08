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

#ifndef MIR_CLIENT_FRAME_CLOCK_H_
#define MIR_CLIENT_FRAME_CLOCK_H_

#include "mir/time/posix_timestamp.h"
#include <chrono>
#include <functional>
#include <mutex>

namespace mir { namespace client {

class FrameClock
{
public:
    typedef std::function<time::PosixTimestamp()> ResyncCallback;
    typedef std::function<time::PosixTimestamp(clockid_t)> GetCurrentTime;

    FrameClock(GetCurrentTime);
    FrameClock() : FrameClock(mir::time::PosixTimestamp::now) {}

    /**
     * Set the precise frame period in nanoseconds (1000000000/Hz).
     */
    void set_period(std::chrono::nanoseconds);

    /**
     * Optionally set a callback that obtains the latest hardware vsync
     * timestamp. This provides phase correction for increased precision but is
     * not strictly required.
     *   Lowest precision: Don't provide a callback.
     *   Medium precision: Provide a callback which returns a recent timestamp.
     *   Highest precision: Provide a callback that queries the server.
     */
    void set_resync_callback(ResyncCallback);

    /**
     * Return the next timestamp to sleep_until, which comes after the last one
     * that was slept till (or more generally after time 'when'). On the first
     * frame you can just provide an uninitialized timestamp.
     */
    time::PosixTimestamp next_frame_after(time::PosixTimestamp when) const;

private:
    time::PosixTimestamp fallback_resync_callback() const;

    GetCurrentTime const get_current_time;

    mutable std::mutex mutex;  // Protects below fields:
    mutable bool config_changed;
    mutable std::chrono::nanoseconds phase;
    std::chrono::nanoseconds period;
    ResyncCallback resync_callback;
};

}} // namespace mir::client

#endif // MIR_CLIENT_FRAME_CLOCK_H_
