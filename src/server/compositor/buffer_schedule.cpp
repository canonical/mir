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

#include "buffer_schedule.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/frontend/event_sink.h"
#include <boost/throw_exception.hpp>
#include <algorithm>

namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace mf = mir::frontend;

mc::BufferSchedule::BufferSchedule(
    mf::BufferStreamId id,
    std::shared_ptr<mf::EventSink> const& sink,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator) :
    stream_id(id),
    sink(sink),
    allocator(allocator)
{
}

void mc::BufferSchedule::add_buffer(graphics::BufferProperties const& properties)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    auto buffer = allocator->alloc_buffer(properties);
    buffers[buffer->id()] = buffer;
    sink->send_buffer(stream_id, *buffer, mg::BufferIpcMsgType::full_msg);
}

mc::BufferSchedule::BufferMap::iterator mc::BufferSchedule::checked_buffers_find(
    mg::BufferID id, std::unique_lock<std::mutex> const&)
{
    auto it = buffers.find(id);
    if (it == buffers.end())
        BOOST_THROW_EXCEPTION(std::logic_error("cannot find buffer by id"));
    return it;
}

void mc::BufferSchedule::remove_buffer(mg::BufferID id)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    buffers.erase(checked_buffers_find(id, lk));
    for (auto& entry : schedule)
    {
        if (entry.buffer->id() == id)
            entry.dead = true;
    }
}

void mc::BufferSchedule::schedule_buffer(mg::BufferID id)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    auto it = checked_buffers_find(id, lk);

    if (!schedule.empty() && schedule.front().use_count == 0)
    {
        sink->send_buffer(stream_id, *schedule.front().buffer, mg::BufferIpcMsgType::update_msg);
        schedule.pop_front();
    }
    
    schedule.emplace_front(ScheduleEntry{it->second, 0, false, false});
}

std::shared_ptr<mg::Buffer> mc::BufferSchedule::compositor_acquire(compositor::CompositorID)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    //might be better to take a buffer in the constructor and never have an empty schedule
    if (schedule.empty())
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer scheduled"));

    auto& last_entry = schedule.front();
    last_entry.was_consumed = true;
    last_entry.use_count++;
    return last_entry.buffer;
}

void mc::BufferSchedule::compositor_release(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::unique_lock<decltype(mutex)> lk(mutex);

    auto it = std::find_if(schedule.begin(), schedule.end(),
        [&buffer](ScheduleEntry const& s) { return s.buffer->id() == buffer->id(); });
    if (it == schedule.end())
        BOOST_THROW_EXCEPTION(std::logic_error("buffer not scheduled"));
    auto& last_entry = *it;
    last_entry.use_count--;

    if ((last_entry.use_count == 0) &&
         last_entry.was_consumed    &&
        (schedule.front().buffer != buffer))
    {
        if (!last_entry.dead)
            sink->send_buffer(stream_id, *last_entry.buffer, mg::BufferIpcMsgType::update_msg);
        schedule.erase(it);
    }
}
