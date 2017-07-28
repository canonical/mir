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

#ifndef MIR_TIME_POSIX_TIMESTAMP_H_
#define MIR_TIME_POSIX_TIMESTAMP_H_

#include <chrono>
#include <ctime>
#include <stdexcept>

namespace mir { namespace time {

/*
 * We need absolute precision here so sadly can't use high-level C++ clocks...
 *  - Graphics frame timing needs support for at least the kernel clocks
 *    CLOCK_REALTIME and CLOCK_MONOTONIC, to be selected at runtime, whereas
 *    std::chrono does not support CLOCK_REALTIME or easily switching clocks.
 *  - mir::time::Timestamp is relative to the (wrong) epoch of steady_clock,
 *    so converting to/from mir::time::Timestamp would be dangerously
 *    inaccurate at best.
 */

struct PosixTimestamp
{
    clockid_t clock_id;
    std::chrono::nanoseconds nanoseconds;

    PosixTimestamp()
        : clock_id{CLOCK_MONOTONIC}, nanoseconds{0} {}
    PosixTimestamp(clockid_t clk, std::chrono::nanoseconds ns)
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

inline void assert_same_clock(PosixTimestamp const& a, PosixTimestamp const& b)
{
    if (a.clock_id != b.clock_id)
        throw std::logic_error("Can't compare different time domains");
}

inline bool operator==(PosixTimestamp const& a, PosixTimestamp const& b)
{
    return a.clock_id == b.clock_id && a.nanoseconds == b.nanoseconds;
}

inline PosixTimestamp operator-(PosixTimestamp const& a,
                                std::chrono::nanoseconds b)
{
    return PosixTimestamp(a.clock_id, a.nanoseconds - b);
}

inline std::chrono::nanoseconds operator-(PosixTimestamp const& a,
                                          PosixTimestamp const& b)
{
    assert_same_clock(a, b);
    return a.nanoseconds - b.nanoseconds;
}

inline PosixTimestamp operator+(PosixTimestamp const& a,
                                std::chrono::nanoseconds b)
{
    return PosixTimestamp(a.clock_id, a.nanoseconds + b);
}

inline std::chrono::nanoseconds operator%(PosixTimestamp const& a,
                                          std::chrono::nanoseconds b)
{
    return std::chrono::nanoseconds(a.nanoseconds.count() % b.count());
}

inline bool operator>(PosixTimestamp const& a, PosixTimestamp const& b)
{
    assert_same_clock(a, b);
    return a.nanoseconds > b.nanoseconds;
}

inline bool operator<(PosixTimestamp const& a, PosixTimestamp const& b)
{
    assert_same_clock(a, b);
    return a.nanoseconds < b.nanoseconds;
}

inline bool operator>=(PosixTimestamp const& a, PosixTimestamp const& b)
{
    assert_same_clock(a, b);
    return a.nanoseconds >= b.nanoseconds;
}

inline bool operator<=(PosixTimestamp const& a, PosixTimestamp const& b)
{
    assert_same_clock(a, b);
    return a.nanoseconds <= b.nanoseconds;
}

inline void sleep_until(PosixTimestamp const& t)
{
    long long ns = t.nanoseconds.count();
    struct timespec ts;
    ts.tv_sec = ns / 1000000000LL;
    ts.tv_nsec = ns % 1000000000LL;
    while (EINTR == clock_nanosleep(t.clock_id, TIMER_ABSTIME, &ts, NULL)) {}
}

}} // namespace mir::time

#endif // MIR_TIME_POSIX_TIMESTAMP_H_
