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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "stream.h"
#include "queueing_schedule.h"
#include "dropping_schedule.h"
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

void mc::Stream::submit_buffer(std::shared_ptr<mg::Buffer> const& buffer)
{
    if (!buffer)
        BOOST_THROW_EXCEPTION(std::invalid_argument("cannot submit null buffer"));

    {
        std::lock_guard<decltype(mutex)> lk(mutex); 
        first_frame_posted = true;
        pf = buffer->pixel_format();
        schedule->schedule(buffer);
    }
    observers.frame_posted(1, buffer->size());
}

void mc::Stream::with_most_recent_buffer_do(std::function<void(mg::Buffer&)> const& fn)
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    fn(*arbiter->snapshot_acquire());
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
    return arbiter->compositor_acquire(id);
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

void mc::Stream::set_scale(float)
{
}
