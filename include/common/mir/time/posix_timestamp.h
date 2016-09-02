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

#ifndef MIR_TIME_POSIX_TIMESTAMP_H_
#define MIR_TIME_POSIX_TIMESTAMP_H_

#include <cstdint>
#include <ctime>

namespace mir { namespace time {

/*
 * Using bare int types because we must maintain these values accurately from
 * the driver code all the way to clients, and clients of clients ad infinitum.
 * int64_t is used because that's what drivers tend to provide and we also need
 * to support negative durations (sometimes things are in the future).
 *   Also note that we use PosixTimestamp instead of std::chrono::time_point as
 * we need to be able to switch clocks dynamically at runtime, depending on
 * which one any given graphics driver claims to use. They will always use one
 * of the kernel clocks...
 */

struct PosixTimestamp
{
    clockid_t clock_id;
    int64_t nanoseconds;

    PosixTimestamp()
        : clock_id{CLOCK_MONOTONIC}, nanoseconds{0} {}
    // Not sure why gcc-4.9 (vivid) demands this. TODO
    PosixTimestamp(clockid_t clk, int64_t ns)
        : clock_id{clk}, nanoseconds{ns} {}
    PosixTimestamp(clockid_t clk, struct timespec const& ts)
        : clock_id{clk}, nanoseconds{ts.tv_sec*1000000000LL + ts.tv_nsec} {}

    static PosixTimestamp now(clockid_t clock_id)
    {
        struct timespec ts;
        clock_gettime(clock_id, &ts);
        return PosixTimestamp(clock_id, ts);
    }
};

}} // namespace mir::time

#endif // MIR_TIME_POSIX_TIMESTAMP_H_
