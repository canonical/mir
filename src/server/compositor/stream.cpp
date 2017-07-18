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
#include "mir/graphics/buffer.h"
#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace geom = mir::geometry;

enum class mc::Stream::ScheduleMode {
    Queueing,
    Dropping
};

mc::Stream::Stream(
    geom::Size size, MirPixelFormat pf) :
    schedule_mode(ScheduleMode::Queueing),
    schedule(std::make_shared<mc::QueueingSchedule>()),
    arbiter(std::make_shared<mc::MultiMonitorArbiter>(schedule)),
    size(size),
    pf(pf),
    first_frame_posted(false)
{
}

mc::Stream::~Stream() = default;

unsigned int mc::Stream::client_owned_buffer_count(std::lock_guard<decltype(mutex)> const&) const
{
    auto server_count = schedule->num_scheduled();
    if (arbiter->has_buffer())
        server_count++;
    return associated_buffers.size() - server_count;
}

void mc::Stream::submit_buffer(std::shared_ptr<mg::Buffer> const& buffer)
{
    std::future<void> deferred_io;

    if (!buffer)
        BOOST_THROW_EXCEPTION(std::invalid_argument("cannot submit null buffer"));

    {
        std::lock_guard<decltype(mutex)> lk(mutex); 
        first_frame_posted = true;
        pf = buffer->pixel_format();
        deferred_io = schedule->schedule_nonblocking(buffer);
    }
    observers.frame_posted(1, buffer->size());

    // Ensure that mutex is not locked while we do this (synchronous!) socket
    // IO. Holding it locked blocks the compositor thread(s) from rendering.
    if (deferred_io.valid())
    {
        // TODO: Throttling of GPU hogs goes here (LP: #1211700, LP: #1665802)
        deferred_io.wait();
    }
}

void mc::Stream::with_most_recent_buffer_do(std::function<void(mg::Buffer&)> const& fn)
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    TemporarySnapshotBuffer buffer(arbiter);
    fn(buffer);
}

MirPixelFormat mc::Stream::pixel_format() const
{
    std::lock_guard<decltype(mutex)> lk(mutex);
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
        transition_schedule(std::make_shared<mc::DroppingSchedule>(), lk);
        schedule_mode = ScheduleMode::Dropping;
    }
    else if (!dropping && schedule_mode == ScheduleMode::Dropping)
    {
        transition_schedule(std::make_shared<mc::QueueingSchedule>(), lk);
        schedule_mode = ScheduleMode::Queueing;
    }
}

bool mc::Stream::framedropping() const
{
    return schedule_mode == ScheduleMode::Dropping;
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

    arbiter->advance_schedule();
}

bool mc::Stream::has_submitted_buffer() const
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    return first_frame_posted;
}

void mc::Stream::associate_buffer(mg::BufferID id)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    associated_buffers.insert(id);
}

void mc::Stream::disassociate_buffer(mg::BufferID id)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    auto it = associated_buffers.find(id);
    if (it != associated_buffers.end())
        associated_buffers.erase(it);
}

void mc::Stream::set_scale(float)
{
}
