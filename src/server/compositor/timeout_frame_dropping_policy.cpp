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

#include "timeout_frame_dropping_policy.h"

#include <assert.h>
#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;

mc::TimeoutFrameDroppingPolicy::TimeoutFrameDroppingPolicy(std::shared_ptr<mir::time::Timer> const& timer,
                                                           std::chrono::milliseconds timeout,
                                                           std::function<void(void)> drop_frame)
    : timeout{timeout},
      pending_swaps{0}
{
    alarm = timer->notify_at(mir::time::Timestamp::max(), [this, drop_frame]
    {
       assert(pending_swaps.load() > 0);
       drop_frame();
       if (--pending_swaps > 0)
           alarm->reschedule_in(this->timeout);
    });
}

void mc::TimeoutFrameDroppingPolicy::swap_now_blocking()
{
    if (pending_swaps++ == 0)
        alarm->reschedule_in(timeout);
}

void mc::TimeoutFrameDroppingPolicy::swap_unblocked()
{
    if (alarm->state() != mir::time::Alarm::Cancelled && alarm->cancel())
        pending_swaps--;
}
