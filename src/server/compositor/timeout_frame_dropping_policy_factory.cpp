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

#include "mir/compositor/frame_dropping_policy.h"

#include "timeout_frame_dropping_policy_factory.h"

#include <cassert>
#include <atomic>
#include <chrono>
#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;

namespace
{
class TimeoutFrameDroppingPolicy : public mc::FrameDroppingPolicy
{
public:
    TimeoutFrameDroppingPolicy(std::shared_ptr<mir::time::Timer> const& timer,
                               std::chrono::milliseconds timeout,
                               std::function<void(void)> drop_frame);

    void swap_now_blocking() override;
    void swap_unblocked() override;

private:
    std::chrono::milliseconds const timeout;
    std::atomic<unsigned int> pending_swaps;

    // Ensure alarm gets destroyed first so its handler does not access dead
    // objects.
    std::unique_ptr<mir::time::Alarm> const alarm;
};

TimeoutFrameDroppingPolicy::TimeoutFrameDroppingPolicy(std::shared_ptr<mir::time::Timer> const& timer,
                                                       std::chrono::milliseconds timeout,
                                                       std::function<void(void)> drop_frame)
    : timeout{timeout},
      pending_swaps{0},
      alarm{timer->create_alarm([this, drop_frame]
        {
            assert(pending_swaps.load() > 0);
            drop_frame();
            if (--pending_swaps > 0)
                alarm->reschedule_in(this->timeout);
        })}
{
}

void TimeoutFrameDroppingPolicy::swap_now_blocking()
{
    if (pending_swaps++ == 0)
        alarm->reschedule_in(timeout);
}

void TimeoutFrameDroppingPolicy::swap_unblocked()
{
    if (alarm->state() != mir::time::Alarm::cancelled && alarm->cancel())
    {
        if (--pending_swaps > 0)
        {
            alarm->reschedule_in(timeout);
        }
    }
}
}

mc::TimeoutFrameDroppingPolicyFactory::TimeoutFrameDroppingPolicyFactory(std::shared_ptr<mir::time::Timer> const& timer,
                                                                         std::chrono::milliseconds timeout)
    : timer{timer},
      timeout{timeout}
{
}


std::unique_ptr<mc::FrameDroppingPolicy> mc::TimeoutFrameDroppingPolicyFactory::create_policy(std::function<void ()> drop_frame) const
{
    return std::unique_ptr<mc::FrameDroppingPolicy>{new TimeoutFrameDroppingPolicy{timer, timeout, drop_frame}};
}
