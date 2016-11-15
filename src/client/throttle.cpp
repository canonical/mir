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

Throttle::Throttle()
    : readjustment_required{false}
{
    set_speed(60);
    set_resync_callback(std::bind(&Throttle::fake_resync_callback, this));
}

void Throttle::set_speed(double hz)
{
    if (hz <= 0.0)
        throw std::logic_error("Throttle::set_speed must be greater than zero");

    interval = std::chrono::nanoseconds(static_cast<long>(1000000000L / hz));
    readjustment_required = true;
}

void Throttle::set_resync_callback(ResyncCallback cb)
{
    resync_callback = cb;
    readjustment_required = true;
}

PosixTimestamp Throttle::fake_resync_callback() const
{
    fprintf(stderr, "fake_resync_callback\n");
    auto const now = PosixTimestamp::now(CLOCK_MONOTONIC);
    return now - (now % interval);
}

PosixTimestamp Throttle::next_frame_after(PosixTimestamp prev) const
{
    auto target = prev + interval;

    /*
     * On the first frame, and any resumption frame (after the client goes
     * from idle to busy) this will trigger a query to the server (or virtual
     * server) to ask for the latest hardware vsync timestamp. So we get
     * in perfect sync with the display, but only need to query the server
     * like this occasionally and not on every frame...
     */
    auto const now = PosixTimestamp::now(target.clock_id);
    if (target < now || readjustment_required)
    {
        readjustment_required = false;
        auto const server_frame = resync_callback();
        auto const server_frame_phase = server_frame % interval;
        auto const now_phase = now % interval;

        // Target the next future time that's in phase with the server's vsync:
        target = now + (server_frame_phase - now_phase);
    }

#if 1
    auto delta = target - prev;
    long usec = delta.count() / 1000;
    fprintf(stderr, "Wait delta %ld.%03ldms\n", usec/1000, usec%1000);
#endif

    return target;
}
