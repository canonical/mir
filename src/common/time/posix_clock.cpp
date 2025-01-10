#include "mir/time/posix_clock.h"
#include <chrono>
#include <ctime>
#include <system_error>
#include <boost/throw_exception.hpp>

namespace mt = mir::time;

auto mt::clock_gettime_checked(clockid_t clock_id) -> std::chrono::nanoseconds
{
    struct timespec ts;
    if (auto err = clock_gettime(clock_id, &ts))
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                err,
                std::system_category(),
                "clock_gettime failed"}));
    }
    return std::chrono::nanoseconds{ts.tv_nsec + ts.tv_sec * std::chrono::nanoseconds::period::den};
}

static_assert(
    std::chrono::is_clock_v<mt::PosixClock<CLOCK_MONOTONIC>>,
    "PosixClock<CLOCK_MONOTONIC> is a Clock type");

static_assert(
    mt::PosixClock<CLOCK_MONOTONIC>::is_steady,
    "PosixClock<CLOCK_MONOTONIC> should be steady");

static_assert(
    !mt::PosixClock<CLOCK_REALTIME>::is_steady,
    "PosixClock<CLOCK_REALTIME> should not be steady");
