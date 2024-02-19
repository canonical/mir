/*
 * Copyright © Canonical Ltd.
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
 */

#include "stream.h"
#include "mir/graphics/buffer.h"
#include <boost/throw_exception.hpp>
#include <math.h>

#include <cmath>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mc::Stream::Stream(
    geom::Size size, MirPixelFormat pf) :
    arbiter(std::make_shared<mc::MultiMonitorArbiter>()),
    latest_buffer_size(size),
    pf(pf),
    first_frame_posted(false),
    frame_callback{[](auto){}}
{
}

mc::Stream::~Stream() = default;

void mc::Stream::submit_buffer(std::shared_ptr<mg::Buffer> const& buffer)
{
    if (!buffer)
        BOOST_THROW_EXCEPTION(std::invalid_argument("cannot submit null buffer"));

    {
        std::lock_guard lk(mutex);
        pf = buffer->pixel_format();
        latest_buffer_size = buffer->size();
        arbiter->submit_buffer(buffer);
        first_frame_posted = true;
    }
    {
        std::lock_guard lock{callback_mutex};
        frame_callback(buffer->size());
    }
}

void mc::Stream::with_most_recent_buffer_do(std::function<void(mg::Buffer&)> const& fn)
{
    std::lock_guard lk(mutex);
    fn(*arbiter->snapshot_acquire());
}

MirPixelFormat mc::Stream::pixel_format() const
{
    std::lock_guard lk(mutex);
    return pf;
}

void mc::Stream::set_frame_posted_callback(
    std::function<void(geometry::Size const&)> const& callback)
{
    std::lock_guard lock{callback_mutex};
    frame_callback = callback;
}

std::shared_ptr<mg::Buffer> mc::Stream::lock_compositor_buffer(void const* id)
{
    return arbiter->compositor_acquire(id);
}

geom::Size mc::Stream::stream_size()
{
    std::lock_guard lk(mutex);
    return geom::Size{
        roundf(latest_buffer_size.width.as_int() / scale_),
        roundf(latest_buffer_size.height.as_int() / scale_)};
}

int mc::Stream::buffers_ready_for_compositor(void const* id) const
{
    std::lock_guard lk(mutex);
    if (arbiter->buffer_ready_for(id))
        return 1;
    return 0;
}

bool mc::Stream::has_submitted_buffer() const
{
    // Don't need to lock mutex because first_frame_posted is atomic
    return first_frame_posted;
}

void mc::Stream::set_scale(float scale)
{
    std::lock_guard lk(mutex);
    scale_ = scale;
}
