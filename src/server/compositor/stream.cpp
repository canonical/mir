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

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;

enum class mc::Stream::ScheduleMode {
    Queueing,
    Dropping
};

mc::Stream::Stream(std::unique_ptr<frontend::ClientBuffers> map, geom::Size size, MirPixelFormat pf) :
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

void mc::Stream::swap_buffers(mg::Buffer* buffer, std::function<void(mg::Buffer* new_buffer)> fn)
{
    if (buffer)
    {
        {
            std::lock_guard<decltype(mutex)> lk(mutex); 
            first_frame_posted = true;
            schedule->schedule((*buffers)[buffer->id()]);
        }
        observers.frame_posted(1);
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
    std::lock_guard<decltype(mutex)> lk(mutex); 
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
    while(schedule->anything_scheduled())
        transferred_buffers.emplace_back(schedule->next_buffer());
    for(auto& buffer : transferred_buffers)
        new_schedule->schedule(buffer);
    schedule = new_schedule;
    arbiter->set_schedule(schedule);
}

void mc::Stream::force_requests_to_complete()
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
    while(schedule->anything_scheduled())
        transferred_buffers.emplace_back(schedule->next_buffer());
    if (!transferred_buffers.empty())
        schedule->schedule(transferred_buffers.front());
}

bool mc::Stream::has_submitted_buffer() const
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    return first_frame_posted;
}

mg::BufferID mc::Stream::allocate_buffer(mg::BufferProperties const& properties)
{
    return buffers->add_buffer(properties);
}

void mc::Stream::remove_buffer(mg::BufferID id)
{
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
