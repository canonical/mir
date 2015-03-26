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
 * Authored by: Robert Carr <racarr@canonical.com>
 */

#include "surfaceless_buffer_stream.h"

#include "mir/compositor/buffer_stream.h"
#include "mir/scene/surface_observer.h"
#include "mir/frontend/buffer_stream.h"

#include "mir_toolkit/client_types.h"

#include <boost/throw_exception.hpp>

#include <stdexcept>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::scene;

ms::SurfacelessBufferStream::SurfacelessBufferStream(std::shared_ptr<mc::BufferStream> const& buffer_stream)
    : buffer_stream(buffer_stream)
{
    buffer_stream->allow_framedropping(true);
}

void ms::SurfacelessBufferStream::swap_buffers(mg::Buffer* old_buffer, std::function<void(mg::Buffer* new_buffer)> complete)
{
    if (old_buffer)
    {
        buffer_stream->release_client_buffer(old_buffer);

        comp_buffer = buffer_stream->lock_compositor_buffer(this);
        if (observer)
        {
            observer->frame_posted(buffer_stream->buffers_ready_for_compositor(this));
        }
        comp_buffer.reset();
    }
    buffer_stream->acquire_client_buffer(complete);
}

void ms::SurfacelessBufferStream::with_most_recent_buffer_do(std::function<void(mg::Buffer&)> const& exec)
{
    // If we supported multiple observers we would need some sort oflock here but as it stands a single observer can only
    // access comp_buffer by calling with_most_recent_buffer_do in the scope of a frames_posted callback so there is
    // no potential for race.
    if (comp_buffer)
        exec(*comp_buffer);
}

void ms::SurfacelessBufferStream::add_observer(std::shared_ptr<ms::SurfaceObserver> const& new_observer)
{
    // TODO: Could utilize ms::SurfaceObservers to enable situations like setting multiple surfaces
    // cursor requests from one buffer stream (We would then need some locking here) ~racarr
    if (observer)
        BOOST_THROW_EXCEPTION(std::runtime_error("Simple buffer stream only supports one observer"));
    observer = new_observer;
}

void ms::SurfacelessBufferStream::remove_observer(std::weak_ptr<ms::SurfaceObserver> const& /* observer */)
{
    observer.reset();
}

MirPixelFormat ms::SurfacelessBufferStream::pixel_format() const
{
    return buffer_stream->get_stream_pixel_format();
}

