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

#ifndef MIR_THROTTLE_H_
#define MIR_THROTTLE_H_

#include "mir/time/posix_timestamp.h"
#include <chrono>
#include <mutex>

namespace mir {

class Throttle
{
public:
    Throttle();
    void set_speed(double hz);
    void set_phase(time::PosixTimestamp const& last_known_vblank);
    time::PosixTimestamp next_frame() const;

    // TODO: remove when we have real server timestamps:
    std::chrono::nanoseconds get_interval() const { return interval; }

private:
    std::mutex mutex;
    std::chrono::nanoseconds interval;
    time::PosixTimestamp last_server_vsync;
    mutable time::PosixTimestamp last_target;
};

} // namespace mir

#endif // MIR_THROTTLE_H_
