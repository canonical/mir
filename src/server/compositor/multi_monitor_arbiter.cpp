/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "multi_monitor_arbiter.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/frontend/event_sink.h"
#include "schedule.h"
#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;

mc::MultiMonitorArbiter::MultiMonitorArbiter(
    std::shared_ptr<Schedule> const& schedule) :
    schedule(schedule)
{
}

mc::MultiMonitorArbiter::~MultiMonitorArbiter()
{
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::compositor_acquire(compositor::CompositorID id)
{
    std::lock_guard<decltype(mutex)> lk(mutex);

    // If there is no current buffer or there is, but this compositor is already using it...
    if (!current_buffer || current_buffer_users.find(id) != current_buffer_users.end())
    {
        // And if there is a scheduled buffer
        if (schedule->num_scheduled() > 0)
        {
            // Advance the current buffer
            current_buffer = schedule->next_buffer();
            current_buffer_users.clear();
        }
        // Otherwise leave the current buffer alone
    }

    // If there was no current buffer and we weren't able to set one, throw and exception
    if (!current_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to compositor"));

    // The compositor is now a user of the current buffer
    // This means we will try to give it a new buffer next time it asks
    current_buffer_users.insert(id);
    return current_buffer;
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::snapshot_acquire()
{
    std::lock_guard<decltype(mutex)> lk(mutex);

    if (!current_buffer)
    {
        if (schedule->num_scheduled() > 0)
        {
            current_buffer = schedule->next_buffer();
            current_buffer_users.clear();
        }
        else
        {
            BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to snapshotter"));
        }
    }

    return current_buffer;
}

void mc::MultiMonitorArbiter::set_schedule(std::shared_ptr<Schedule> const& new_schedule)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    schedule = new_schedule;
}

bool mc::MultiMonitorArbiter::buffer_ready_for(mc::CompositorID id)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    // If there are scheduled buffers then there is one ready for any compositor
    if (schedule->num_scheduled() > 0)
        return true;
    // If we have a current buffer that the compositor isn't yet using, it is ready
    else if (current_buffer && current_buffer_users.find(id) == current_buffer_users.end())
        return true;
    // There are no schedualed buffers and either no current buffer, or a current buffer already used by this compositor
    else
        return false;
}

void mc::MultiMonitorArbiter::advance_schedule()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    if (schedule->num_scheduled() > 0)
    {
        current_buffer = schedule->next_buffer();
        current_buffer_users.clear();
    } 
}
