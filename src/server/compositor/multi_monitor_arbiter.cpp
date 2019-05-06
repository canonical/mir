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

    if (!current_buffer && !schedule->num_scheduled())
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to compositor"));

    if (current_buffer_users.find(id) != current_buffer_users.end() || !current_buffer)
    {
        if (schedule->num_scheduled())
            current_buffer = schedule->next_buffer();
        current_buffer_users.clear();
    }
    current_buffer_users.insert(id);

    return current_buffer;
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::snapshot_acquire()
{
    std::lock_guard<decltype(mutex)> lk(mutex);

    if (!current_buffer && !schedule->num_scheduled())
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to snapshotter"));

    if (!current_buffer)
    {
        if (schedule->num_scheduled())
            current_buffer = schedule->next_buffer();
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
    return schedule->num_scheduled() ||
       ((current_buffer_users.find(id) == current_buffer_users.end()) && current_buffer);
}

void mc::MultiMonitorArbiter::advance_schedule()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    if (schedule->num_scheduled())
    {
        current_buffer = schedule->next_buffer();
        current_buffer_users.clear();
    } 
}
