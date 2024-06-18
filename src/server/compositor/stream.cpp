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

#include "mir/compositor/stream.h"
#include "multi_monitor_arbiter.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/buffer.h"
#include <boost/throw_exception.hpp>
#include <math.h>

#include <cmath>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mc::Stream::Stream() :
    arbiter(std::make_shared<mc::MultiMonitorArbiter>()),
    first_frame_posted(false),
    frame_callback{[](auto){}}
{
}

mc::Stream::~Stream() = default;

void mc::Stream::submit_buffer(
    std::shared_ptr<mg::Buffer> const& buffer,
    geom::Size dst_size,
    geom::RectangleD src_bounds)
{
    if (!buffer)
        BOOST_THROW_EXCEPTION(std::invalid_argument("cannot submit null buffer"));

    arbiter->submit_buffer(buffer, dst_size, src_bounds);
    first_frame_posted = true;
    {
        (*frame_callback.lock())(buffer->size());
    }
}

void mc::Stream::set_frame_posted_callback(
    std::function<void(geometry::Size const&)> const& callback)
{
    *frame_callback.lock() = callback;
}

auto mc::Stream::next_submission_for_compositor(void const* id) -> std::shared_ptr<Submission>
{
    return arbiter->compositor_acquire(id);
}

bool mc::Stream::has_submitted_buffer() const
{
    // Don't need to lock mutex because first_frame_posted is atomic
    return first_frame_posted;
}
