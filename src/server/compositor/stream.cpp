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
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace ms = mir::scene;

mc::Stream::Stream(frontend::BufferStreamId, std::shared_ptr<frontend::EventSink> const&) :
    first_frame_posted(false)
{
}

void mc::Stream::swap_buffers(mg::Buffer* buffer, std::function<void(mg::Buffer* new_buffer)>)
{
    if (!buffer) return;
    std::lock_guard<decltype(mutex)> lk(mutex); 
    first_frame_posted = true;
    observers.frame_posted(1);
}

void mc::Stream::with_most_recent_buffer_do(std::function<void(mg::Buffer&)> const&)
{
}

MirPixelFormat mc::Stream::pixel_format() const
{
    return mir_pixel_format_abgr_8888;
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

std::shared_ptr<mg::Buffer> mc::Stream::lock_compositor_buffer(void const*)
{
    return nullptr;
}

geom::Size mc::Stream::stream_size()
{
    return {0,0};
}

void mc::Stream::resize(geom::Size const&)
{
}

void mc::Stream::allow_framedropping(bool)
{
}

void mc::Stream::force_requests_to_complete()
{
}

int mc::Stream::buffers_ready_for_compositor(void const*) const
{
    return 8;
}

void mc::Stream::drop_old_buffers()
{
}

bool mc::Stream::has_submitted_buffer() const
{
    std::lock_guard<decltype(mutex)> lk(mutex); 
    return first_frame_posted;
}
