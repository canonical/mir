
/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_COMMON_TIME_POSIX_CLOCK_
#define MIR_COMMON_TIME_POSIX_CLOCK_

#include <chrono>
#include <ctime>

namespace mir::time
{
template<clockid_t clock_id>
struct is_steady_specialisation
{
    static constexpr bool is_steady = false;
};

template<>
struct is_steady_specialisation<CLOCK_MONOTONIC>
{
    static constexpr bool is_steady = true;
};

template<>
struct is_steady_specialisation<CLOCK_MONOTONIC_RAW>
{
    static constexpr bool is_steady = true;
};

template<>
struct is_steady_specialisation<CLOCK_MONOTONIC_COARSE>
{
    static constexpr bool is_steady = true;
};

auto clock_gettime_checked(clockid_t clock_id) -> std::chrono::nanoseconds;

template<clockid_t clock_id>
class PosixClock : public is_steady_specialisation<clock_id>
{
public:
    typedef std::chrono::nanoseconds  duration;
    typedef duration::rep rep;
    typedef duration::period period;
    typedef std::chrono::time_point<PosixClock, duration> time_point;

    static auto now() -> time_point
    {
        return time_point{clock_gettime_checked(clock_id)};
    }
};
}

#endif // MIR_COMMON_TIME_POSIX_CLOCK_
