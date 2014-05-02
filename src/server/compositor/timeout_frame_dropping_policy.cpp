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
                                                           std::chrono::milliseconds timeout)
    : timer{timer},
      timeout{timeout}
{
}

mc::FrameDroppingPolicy::Cookie* mc::TimeoutFrameDroppingPolicy::swap_now_blocking(std::function<void(void)> drop_frame)
{
    if (alarm)
        BOOST_THROW_EXCEPTION(std::logic_error("Scheduling framedrop, but a framedrop is already pending"));

    alarm = timer->notify_in(timeout, [this, drop_frame](){ drop_frame(); alarm.reset(); });
    return reinterpret_cast<Cookie*>(alarm.get());
}

void mc::TimeoutFrameDroppingPolicy::swap_unblocked(mc::FrameDroppingPolicy::Cookie* cookie)
{
    assert(reinterpret_cast<mir::time::Alarm*>(cookie) == alarm.get());
    alarm.reset();
}
