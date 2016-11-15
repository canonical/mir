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
    set_speed(60);
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

PosixTimestamp Throttle::next_frame_after(PosixTimestamp prev) const
{
    auto target = prev + interval;
    if (target < PosixTimestamp::now(target.clock_id))
    {   // The server got ahead of us. That's normal as most clients don't need
        // to render constantly.

        // TODO: Replace this with get_last_server_vsync, so finally we
        //       have a means to avoid a round trip on most frames.
        target = last_server_vsync;
    }

#if 1
    auto delta = target - prev;
    long usec = delta.count() / 1000;
    fprintf(stderr, "Wait delta %ld.%03ldms\n", usec/1000, usec%1000);
#endif

    return target;
}
