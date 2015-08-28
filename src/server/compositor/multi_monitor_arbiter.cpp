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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
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
    PresentationGuarantee guarantee,
    std::shared_ptr<frontend::ClientBuffers> const& map,
    std::shared_ptr<Schedule> const& schedule) :
    guarantee(guarantee),
    map(map),
    schedule(schedule)
{
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::compositor_acquire(compositor::CompositorID id)
{
    std::lock_guard<decltype(mutex)> lk(mutex);

    if (!current && !schedule->anything_scheduled())
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to compositor"));

    if (current_buffer_users.find(id) != current_buffer_users.end() || !current)
    {
        if (schedule->anything_scheduled())
        {
        current = schedule->next_buffer();
        onscreen_buffers.emplace_front(current, 0);
        current_buffer_users.clear();
        }
    }
    current_buffer_users.insert(id);

    for(auto it = onscreen_buffers.begin(); it != onscreen_buffers.end();)
    {
        if (it->buffer == current)
            it->use_count++;

        if ((it->use_count == 0) && (guarantee != mc::PresentationGuarantee::frames_on_any_monitor))
        {
            map->send_buffer(it->buffer->id());
            it = onscreen_buffers.erase(it);
        }
        else
        {
            it++;
        }
    }

    return current;
}

void mc::MultiMonitorArbiter::compositor_release(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::lock_guard<decltype(mutex)> lk(mutex);

    for(auto it = onscreen_buffers.begin(); it != onscreen_buffers.end();)
    {
        if (buffer == it->buffer)
            it->use_count--;

        if ((it->use_count == 0) &&
            (current != it->buffer || (schedule->anything_scheduled())) &&
            (guarantee == mc::PresentationGuarantee::frames_on_any_monitor))
        {
            map->send_buffer(it->buffer->id());
            it = onscreen_buffers.erase(it);
        }
        else
        {
            it++;
        } 
    }
}

std::shared_ptr<mg::Buffer> mc::MultiMonitorArbiter::snapshot_acquire()
{
    std::lock_guard<decltype(mutex)> lk(mutex);

    if (onscreen_buffers.empty() && !schedule->anything_scheduled())
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give to snapshotter"));

    if (onscreen_buffers.empty())
    {
        if (schedule->anything_scheduled())
            onscreen_buffers.emplace_front(schedule->next_buffer(), 0);
    }

    auto& last_entry = onscreen_buffers.front();
    last_entry.use_count++;
    return last_entry.buffer;
}

void mc::MultiMonitorArbiter::snapshot_release(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    for(auto it = onscreen_buffers.begin(); it != onscreen_buffers.end();)
    {
        if (buffer == it->buffer)
            it->use_count--;

        if ((it->use_count == 0) &&
            (current != it->buffer || (schedule->anything_scheduled())))
        {
            map->send_buffer(it->buffer->id());
            it = onscreen_buffers.erase(it);
        }
        else
        {
            it++;
        } 
    }
}

void mc::MultiMonitorArbiter::set_schedule(std::shared_ptr<Schedule> const& new_schedule)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    schedule = new_schedule;
}

void mc::MultiMonitorArbiter::set_guarantee(PresentationGuarantee new_guarantee)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    guarantee = new_guarantee;
}
