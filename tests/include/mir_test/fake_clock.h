/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_FAKE_CLOCK_H_
#define MIR_TEST_FAKE_CLOCK_H_

#include <chrono>
#include <functional>
#include <list>

namespace mir
{
namespace test
{
/**
 * @brief An invasive time source for Mocks/Stubs/Fakes that depend on timing
 */
class FakeClock
{
public:
    typedef std::chrono::nanoseconds duration;
    typedef duration::rep rep;
    typedef duration::period period;
    typedef std::chrono::time_point<FakeClock, duration> time_point;

    static constexpr bool is_steady = false;
    time_point now() const;

    FakeClock();
    /**
     * \brief Advance the fake clock
     * \note Advancing by a negative duration will move the clock backwards
     */
    template<typename rep, typename period>
    void advance_time(std::chrono::duration<rep, period> by)
    {
        advance_time_ns(std::chrono::duration_cast<std::chrono::nanoseconds>(by));
    }

    /**
     * \brief Register an event callback when the time is changed
     * \param cb    Function to call when the time is changed.
     *              This function is called with the new time.
     *              If the function returns false, it will no longer be called
     *              on subsequent time changes.
     */
    void register_time_change_callback(std::function<bool(time_point)> cb);
private:
    void advance_time_ns(std::chrono::nanoseconds by);
    std::chrono::nanoseconds current_time;
    std::list<std::function<bool(time_point)>> callbacks;
};

}
}

#endif // MIR_TEST_FAKE_CLOCK_H_
