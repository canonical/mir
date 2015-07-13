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
    std::shared_ptr<compositor::ClientBufferMap> const& map,
    std::unique_ptr<Schedule> schedule) :
    map(map),
    schedule(std::move(schedule))
{
}

#if 0
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
    auto buffer_it = checked_buffers_find(id, lk);
    schedule->remove(buffer_it->second);
    buffers.erase(buffer_it);

    for (auto& entry : backlog)
    {
        if (entry.buffer->id() == id)
            entry.dead = true;
    }
}
#endif
void mc::BufferSchedule::schedule_buffer(mg::BufferID id)
{
    std::unique_lock<decltype(mutex)> lk(mutex);
    auto buffer = (*map)[id];
//    auto it = checked_buffers_find(id, lk);
//    schedule->schedule(it->second);
}

std::shared_ptr<mg::Buffer> mc::BufferSchedule::compositor_acquire(compositor::CompositorID id)
{
    std::unique_lock<decltype(mutex)> lk(mutex);

    if (backlog.empty() && !schedule->anything_scheduled())
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer to give"));

    if (current_buffer_users.find(id) != current_buffer_users.end() || backlog.empty())
    {
        if (schedule->anything_scheduled())
            backlog.emplace_front(ScheduleEntry{schedule->next_buffer(), 0, false, false});
        current_buffer_users.clear();
    }
    current_buffer_users.insert(id);

    auto& last_entry = backlog.front();
    last_entry.was_consumed = true;
    last_entry.use_count++;
    return last_entry.buffer;
}

void mc::BufferSchedule::compositor_release(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::unique_lock<decltype(mutex)> lk(mutex);

    auto it = std::find_if(backlog.begin(), backlog.end(),
        [&buffer](ScheduleEntry const& s) { return s.buffer->id() == buffer->id(); });
    if (it == backlog.end())
        BOOST_THROW_EXCEPTION(std::logic_error("buffer not scheduled"));

    it->use_count--;;

    clean_backlog();
}

void mc::BufferSchedule::clean_backlog()
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
            //if (map->exists(it->buffer->id())
            //  sink->send_buffer()
//            if (!it->dead)
//            {
//                sink->send_buffer(stream_id, *it->buffer, mg::BufferIpcMsgType::update_msg);
//            }
            map->send_buffer(it->buffer->id());
            it = backlog.erase(it);
        }
        else
        {
            it++;
        } 
    }
}
























////MOVE TO OWN FILE
void mc::QueueingSchedule::remove(std::shared_ptr<mg::Buffer> const& buffer)
{
    auto it = std::find(queue.begin(), queue.end(), buffer);
    if (it != queue.end()) queue.erase(it);
}

void mc::QueueingSchedule::schedule(std::shared_ptr<mg::Buffer> const& buffer)
{

/*
 *  scheduler->schedule(it);
 */
#if 0
    if (!schedule.empty() &&
        schedule.front().was_consumed &&
        schedule.front().use_count == 0)
    {
        sink->send_buffer(stream_id, *schedule.front().buffer, mg::BufferIpcMsgType::update_msg);
        schedule.pop_front();
    }

    schedule.emplace_front(ScheduleEntry{it->second, 0, false, false});
#endif
    queue.emplace_back(buffer);
}

bool mc::QueueingSchedule::anything_scheduled()
{
    return !queue.empty();
}

std::shared_ptr<mg::Buffer> mc::QueueingSchedule::next_buffer()
{
    auto buffer = queue.front();
    queue.pop_front();
    return buffer;
}




















mc::BufferMap::BufferMap(
    mf::BufferStreamId id,
    std::shared_ptr<mf::EventSink> const& sink,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator) :
    stream_id(id),
    sink(sink),
    allocator(allocator)
{
}

void mc::BufferMap::add_buffer(mg::BufferProperties const& properties)
{
    (void) properties;
}

void mc::BufferMap::remove_buffer(mg::BufferID id)
{
    (void) id;
}

void mc::BufferMap::send_buffer(mg::BufferID id)
{
    (void) id;
}

std::shared_ptr<mg::Buffer>& mc::BufferMap::operator[](mg::BufferID id)
{
    (void) id;
    static std::shared_ptr<mg::Buffer> b;
    return b;
}
