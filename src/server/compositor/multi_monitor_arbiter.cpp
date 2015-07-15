/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 */

#include "multi_monitor_arbiter.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/frontend/event_sink.h"
#include "mir/frontend/client_buffers.h"
#include "schedule.h"
#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;

mc::MultiMonitorArbiter::MultiMonitorArbiter(
    std::shared_ptr<frontend::ClientBuffers> const& map,
    std::shared_ptr<Schedule> const& schedule) :
    map(map),
    schedule(schedule)
{
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::compositor_acquire(compositor::CompositorID id)
{
    std::unique_lock<decltype(mutex)> lk(mutex);

    if (backlog.empty() && !schedule->anything_scheduled())
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give"));

    if (current_buffer_users.find(id) != current_buffer_users.end() || backlog.empty())
    {
        if (schedule->anything_scheduled())
        {
            backlog.emplace_front(ScheduleEntry{schedule->next_buffer(), 0, false});
        }
        current_buffer_users.clear();
    }
    current_buffer_users.insert(id);

    auto& last_entry = backlog.front();
    last_entry.was_consumed = true;
    last_entry.use_count++;
    return last_entry.buffer;
}

void mc::MultiMonitorArbiter::compositor_release(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::unique_lock<decltype(mutex)> lk(mutex);

    auto it = std::find_if(backlog.begin(), backlog.end(),
        [&buffer](ScheduleEntry const& s) { return s.buffer->id() == buffer->id(); });
    if (it == backlog.end())
        BOOST_THROW_EXCEPTION(std::logic_error("buffer not scheduled"));

    it->use_count--;;

    clean_backlog();
}

void mc::MultiMonitorArbiter::clean_backlog()
{
    for(auto it = backlog.begin(); it != backlog.end();)
    {
        if (it == backlog.begin() && !schedule->anything_scheduled()) //reserve the front for us
        {
            it++;
            continue;
        }

        if ((it->use_count == 0) &&
            (it->was_consumed))
        {
            map->send_buffer(it->buffer->id());
            it = backlog.erase(it);
        }
        else
        {
            it++;
        } 
    }
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::snapshot_acquire()
{
    BOOST_THROW_EXCEPTION(std::logic_error("not yet implemented"));
}

void mc::MultiMonitorArbiter::snapshot_release(std::shared_ptr<mg::Buffer> const&)
{
}
