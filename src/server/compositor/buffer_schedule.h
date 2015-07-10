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

#ifndef MIR_BUFFER_SCHEDULE_H_
#define MIR_BUFFER_SCHEDULE_H_

#include "mir/frontend/buffer_stream_id.h"
#include "mir/graphics/buffer_id.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/compositor/compositor_id.h"
#include <memory>
#include <map>
#include <mutex>
#include <deque>
#include <set>

namespace mir
{
namespace graphics { class Buffer; class GraphicBufferAllocator; class BufferProperties; }
namespace frontend { class EventSink; }
namespace compositor
{

#if 0
class SchedulingGuarantee
{
    SchedulingGuarantee(std::shared_ptr<mf::EventSink>);
    void eject(std::shared_ptr<> const& buffer)
    {
        event_sink->eject();
    } 

    void schedule(std::shared_ptr<> const& buffer) = 0;
    bool anything_scheduled() = 0;
    std::shared_ptr<Buffer> next_buffer() = 0;
};

//selects a buffer to eject, keeping only one buffer around
class FramedroppingSchedule : SchedulingGuarantee
{
    FramedroppingSchedule(std::shared_ptr<mg::Buffer> const& a_buffer) : 
        the_only_buffer(a_buffer)
    {
    }

    void schedule(std::shared_ptr<mg::Buffer> const& buffer)
    {
        if (the_only_buffer)
            eject(the_only_buffer);
        the_only_buffer = buffer;
    }
 
    std::shared_ptr<mg::Buffer> pop_schedule()
    {
        return the_only_buffer;
    }
    bool anything_scheduled()
    {
        return the_only_buffer;
    }
    std::shared_ptr<mg::Buffer> the_only_buffer;
};

//will queue buffers in the order that they're received and make sure that they're all consumed
class PerfectQueuingSchedule : SchedulingGuarantee
{
    void schedule(std::shared_ptr<mg::Buffer> const& buffer)
    {
        queue.emplace_back(buffer);
    } 
    std::shared_ptr<mg::Buffer> pop_schedule()
    {
        auto b = queue.front();
        queue.pop_front();
        return b;
    }
    bool anything_scheduled()
    {
        return queue.empty();
    }
    std::deque<std::shared_ptr<mg::Buffer>> queue;
};


//will queue buffers in the order that they're received and make sure that they're all consumed.
//if the compositor fails to consume anything in a long time, we'll selectively remove a buffer
//mostly a compromise for Qt
class TimeoutQueueingSchedule : PerfectQueueingSchedule
{
    Timeout()
        policy(make_policy)

    schedule()
    {
        swap_now_blocking();
        schedule();
    }

    pop_schedule()
    {
        swap_unblocked
    }
};

//will ensure that the buffer gets consumed after a specific timepoint
//class TimedSchedule
//{
//    TimedSchedule(std::shared_ptr<time::Clock> const&);
//};

#endif

class Schedule
{
public:
    virtual void schedule(std::shared_ptr<graphics::Buffer> const& buffer) = 0;
    virtual void remove(std::shared_ptr<graphics::Buffer> const& buffer) = 0;
    virtual bool anything_scheduled() = 0;
    virtual std::shared_ptr<graphics::Buffer> next_buffer() = 0;

    virtual ~Schedule() = default;
    Schedule() = default;
    Schedule(Schedule const&) = delete;
    Schedule& operator=(Schedule const&) = delete;
};

class QueueingSchedule : public Schedule
{
public:
    QueueingSchedule(){}
    void schedule(std::shared_ptr<graphics::Buffer> const& buffer) override;
    void remove(std::shared_ptr<graphics::Buffer> const& buffer) override;
    bool anything_scheduled() override;
    std::shared_ptr<graphics::Buffer> next_buffer() override;
private:
    std::deque<std::shared_ptr<graphics::Buffer>> queue;
};

class BufferSchedule
{
public:
    BufferSchedule(
        frontend::BufferStreamId id,
        std::shared_ptr<frontend::EventSink> const& sink,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::unique_ptr<Schedule> schedule);

    void add_buffer(graphics::BufferProperties const& properties);
    void remove_buffer(graphics::BufferID id);

    void schedule_buffer(graphics::BufferID id);

    std::shared_ptr<graphics::Buffer> compositor_acquire(compositor::CompositorID id);
    void compositor_release(std::shared_ptr<graphics::Buffer> const&);

private:
    std::mutex mutable mutex;
    frontend::BufferStreamId const stream_id;
    std::shared_ptr<frontend::EventSink> const sink;
    std::shared_ptr<graphics::GraphicBufferAllocator> const allocator;

    typedef std::map<graphics::BufferID, std::shared_ptr<graphics::Buffer>> BufferMap;
    //used to keep strong reference
    BufferMap buffers;
    BufferMap::iterator checked_buffers_find(graphics::BufferID, std::unique_lock<std::mutex> const&);

    struct ScheduleEntry
    {
        std::shared_ptr<graphics::Buffer> buffer;
        unsigned int use_count;
        bool was_consumed;
        bool dead;
    };
    std::deque<ScheduleEntry> backlog;

    std::set<compositor::CompositorID> current_buffer_users;

    std::unique_ptr<Schedule> const schedule;

    void clean_backlog();
};

}
}

#endif
