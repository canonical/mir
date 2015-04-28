/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "mir/compositor/frame_dropping_policy.h"
#include "mir/lockable_callback_wrapper.h"

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
    TimeoutFrameDroppingPolicy(std::shared_ptr<mir::time::AlarmFactory> const& factory,
                               std::chrono::milliseconds timeout,
                               std::shared_ptr<mir::LockableCallback> const& drop_frame);

    void swap_now_blocking() override;
    void swap_unblocked() override;

private:
    std::chrono::milliseconds const timeout;
    std::atomic<unsigned int> pending_swaps;

    // Ensure alarm gets destroyed first so its handler does not access dead
    // objects.
    std::unique_ptr<mir::time::Alarm> const alarm;
};

TimeoutFrameDroppingPolicy::TimeoutFrameDroppingPolicy(std::shared_ptr<mir::time::AlarmFactory> const& factory,
                                                       std::chrono::milliseconds timeout,
                                                       std::shared_ptr<mir::LockableCallback> const& callback)
    : timeout{timeout},
      pending_swaps{0},
      alarm{factory->create_alarm(
          std::make_shared<mir::LockableCallbackWrapper>(callback,
              [this] { assert(pending_swaps.load() > 0); },
              [this] { if (--pending_swaps > 0) alarm->reschedule_in(this->timeout);} ))}
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

mc::TimeoutFrameDroppingPolicyFactory::TimeoutFrameDroppingPolicyFactory(
    std::shared_ptr<mir::time::AlarmFactory> const& timer,
    std::chrono::milliseconds timeout)
    : factory{timer},
      timeout{timeout}
{
}

std::unique_ptr<mc::FrameDroppingPolicy>
mc::TimeoutFrameDroppingPolicyFactory::create_policy(std::shared_ptr<LockableCallback> const& drop_frame) const
{
    return std::make_unique<TimeoutFrameDroppingPolicy>(factory, timeout, drop_frame);
}
