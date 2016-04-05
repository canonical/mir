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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "stream.h"
#include "queueing_schedule.h"
#include "dropping_schedule.h"
#include "temporary_buffers.h"
#include "mir/frontend/client_buffers.h"
#include "mir/graphics/buffer.h"
#include "mir/compositor/frame_dropping_policy_factory.h"
#include "mir/compositor/frame_dropping_policy.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;

enum class mc::Stream::ScheduleMode {
    Queueing,
    Dropping
};

mc::Stream::DroppingCallback::DroppingCallback(Stream* stream) :
    stream(stream)
{
}

void mc::Stream::DroppingCallback::operator()()
{
    stream->drop_frame();
}

void mc::Stream::DroppingCallback::lock()
{
    guard_lock = std::unique_lock<std::mutex>{stream->mutex};
}

void mc::Stream::DroppingCallback::unlock()
{
    if (guard_lock.owns_lock())
        guard_lock.unlock();
}

mc::Stream::Stream(
    mc::FrameDroppingPolicyFactory const& policy_factory,
    std::unique_ptr<frontend::ClientBuffers> map, geom::Size size, MirPixelFormat pf) :
    drop_policy(policy_factory.create_policy(std::make_unique<DroppingCallback>(this))),
    schedule_mode(ScheduleMode::Queueing),
    schedule(std::make_shared<mc::QueueingSchedule>()),
    buffers(std::move(map)),
    arbiter(std::make_shared<mc::MultiMonitorArbiter>(
        mc::MultiMonitorMode::multi_monitor_sync, buffers, schedule)),
    size(size),
    pf(pf),
    first_frame_posted(false)
{
}

unsigned int mc::Stream::client_owned_buffer_count(std::lock_guard<decltype(mutex)> const&) const
{
    auto server_count = schedule->num_scheduled();
    if (arbiter->has_buffer())
        server_count++;
    return total_buffer_count - server_count;
}

void mc::Stream::swap_buffers(mg::Buffer* buffer, std::function<void(mg::Buffer* new_buffer)> fn)
{
    if (buffer)
    {
        {
            std::lock_guard<decltype(mutex)> lk(mutex); 
            first_frame_posted = true;
            buffers->receive_buffer(buffer->id());
            schedule->schedule((*buffers)[buffer->id()]);
            if (client_owned_buffer_count(lk) == 0)
                drop_policy->swap_now_blocking();
        }
        observers.frame_posted(1, buffer->size());
    }
    fn(nullptr); //legacy support
}

void mc::Stream::with_most_recent_buffer_do(std::function<void(mg::Buffer&)> const& fn)
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    TemporarySnapshotBuffer buffer(arbiter);
    fn(buffer);
}

MirPixelFormat mc::Stream::pixel_format() const
{
    return pf;
}

void mc::Stream::add_observer(std::shared_ptr<ms::SurfaceObserver> const& observer)
{
    observers.add(observer);
}

void mc::Stream::remove_observer(std::weak_ptr<ms::SurfaceObserver> const& observer)
{
    if (auto o = observer.lock())
        observers.remove(o);
}

std::shared_ptr<mg::Buffer> mc::Stream::lock_compositor_buffer(void const* id)
{
    {
        std::lock_guard<decltype(mutex)> lk(mutex);
        drop_policy->swap_unblocked();
    }
    return std::make_shared<mc::TemporaryCompositorBuffer>(arbiter, id);
}

geom::Size mc::Stream::stream_size()
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    return size;
}

void mc::Stream::resize(geom::Size const& new_size)
{
    //TODO: the client should be resizing itself via the buffer creation/destruction rpc calls
    std::lock_guard<decltype(mutex)> lk(mutex);
    size = new_size; 
}

void mc::Stream::allow_framedropping(bool dropping)
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    if (dropping && schedule_mode == ScheduleMode::Queueing)
    {
        transition_schedule(std::make_shared<mc::DroppingSchedule>(buffers), lk);
        schedule_mode = ScheduleMode::Dropping;
    }
    else if (!dropping && schedule_mode == ScheduleMode::Dropping)
    {
        transition_schedule(std::make_shared<mc::QueueingSchedule>(), lk);
        schedule_mode = ScheduleMode::Queueing;
    }
}

void mc::Stream::transition_schedule(
    std::shared_ptr<mc::Schedule>&& new_schedule, std::lock_guard<std::mutex> const&)
{
    std::vector<std::shared_ptr<mg::Buffer>> transferred_buffers;
    while(schedule->num_scheduled())
        transferred_buffers.emplace_back(schedule->next_buffer());
    for(auto& buffer : transferred_buffers)
        new_schedule->schedule(buffer);
    schedule = new_schedule;
    arbiter->set_schedule(schedule);
}

void mc::Stream::drop_outstanding_requests()
{
    //we dont block any requests in this system, nothing to force
}

int mc::Stream::buffers_ready_for_compositor(void const* id) const
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    if (arbiter->buffer_ready_for(id))
        return 1;
    return 0;
}

void mc::Stream::drop_old_buffers()
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    std::vector<std::shared_ptr<mg::Buffer>> transferred_buffers;
    while(schedule->num_scheduled())
        transferred_buffers.emplace_back(schedule->next_buffer());

    if (!transferred_buffers.empty())
    {
        schedule->schedule(transferred_buffers.back());
        transferred_buffers.pop_back();
    }

    for (auto &buffer : transferred_buffers)
        buffers->send_buffer(buffer->id());

    arbiter->advance_schedule();
}

bool mc::Stream::has_submitted_buffer() const
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    return first_frame_posted;
}

mg::BufferID mc::Stream::allocate_buffer(mg::BufferProperties const& properties)
{
    {
        std::lock_guard<decltype(mutex)> lk(mutex); 
        total_buffer_count++;
    }
    return buffers->add_buffer(properties);
}

void mc::Stream::remove_buffer(mg::BufferID id)
{
    {
        std::lock_guard<decltype(mutex)> lk(mutex); 
        total_buffer_count--;
    }
    buffers->remove_buffer(id);
}

void mc::Stream::with_buffer(mg::BufferID id, std::function<void(mg::Buffer&)> const& fn)
{
    auto buffer = (*buffers)[id];
    fn(*buffer);
}

void mc::Stream::set_scale(float)
{
}

void mc::Stream::drop_frame()
{
    if (schedule->num_scheduled() > 1)
        buffers->send_buffer(schedule->next_buffer()->id());
}
