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

using namespace mir;
using namespace mir::time;

Throttle::Throttle()
{
    // A sane default in case we never got told a real rate;
    set_speed(59.9);
}

void Throttle::set_speed(double hz)
{
    std::lock_guard<std::mutex> lock(mutex);
    if (hz > 0.0)
        interval = std::chrono::nanoseconds(static_cast<long>(1000000000L / hz));
    // else warning? Or don't bother because it will be common for virtual
    // machines to report zero.
}

void Throttle::set_phase(PosixTimestamp const& last_known_vblank)
{
    std::lock_guard<std::mutex> lock(mutex);
    last_server_vsync = last_known_vblank;
}

PosixTimestamp Throttle::next_frame() const
{
    /*
     * The period of each client frame should match interval, regardless
     * of the delta of last_server_vsync. Because measuring deltas would be
     * inaccurate on a variable framerate display. Whereas interval
     * represents the theoretical maximum refresh rate of the display we should
     * be aiming for.
     *   The phase however comes from last_server_vsync. This may sound
     * unnecessary but being out of phase with the server (physical display)
     * could create almost one whole frame of extra lag. So it's worth getting
     * in phase with the server regularly...
     */
    auto const server_phase = last_server_vsync % interval;
    auto const last_phase = last_target % interval;
    auto const phase_correction = server_phase - last_phase;
    auto target = last_target + interval + phase_correction;
    if (target < PosixTimestamp::now(target.clock_id))
        target = last_server_vsync;

#if 1
    auto delta = target - last_target;
    long usec = delta.count() / 1000;
    fprintf(stderr, "Wait delta %ld.%03ldms\n", usec/1000, usec%1000);
    fprintf(stderr, "Phase correction %ldus\n",
        (long)(phase_correction.count() / 1000));
#endif

    last_target = target;
    return target;
}
