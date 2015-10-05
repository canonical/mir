/*
 * Copyright © 2012 Canonical Ltd.
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

#include "buffer_stream_surfaces.h"
#include "buffer_bundle.h"
#include "mir/graphics/buffer_properties.h"

#include "temporary_buffers.h"
#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mc::BufferStreamSurfaces::BufferStreamSurfaces(std::shared_ptr<BufferBundle> const& buffer_bundle) :
    buffer_bundle(buffer_bundle),
    first_frame_posted{false},
    logical_size(buffer_bundle->properties().size),
    scale{1.0f}
{
}

mc::BufferStreamSurfaces::~BufferStreamSurfaces()
{
    buffer_bundle->drop_client_requests();
    force_requests_to_complete();
}

std::shared_ptr<mg::Buffer> mc::BufferStreamSurfaces::lock_compositor_buffer(
    void const* user_id)
{
    return std::make_shared<mc::TemporaryCompositorBuffer>(
        buffer_bundle, user_id);
}

void mc::BufferStreamSurfaces::acquire_client_buffer(
    std::function<void(graphics::Buffer* buffer)> complete)
{
    buffer_bundle->client_acquire(complete);
}

void mc::BufferStreamSurfaces::release_client_buffer(graphics::Buffer* buf)
{
    buffer_bundle->client_release(buf);
    {
        std::unique_lock<std::mutex> lk(mutex);
        first_frame_posted = true;
    }
}

geom::Size mc::BufferStreamSurfaces::stream_size()
{
    std::unique_lock<std::mutex> lk(mutex);
    return logical_size;
}

void mc::BufferStreamSurfaces::resize(geom::Size const& size)
{
    std::unique_lock<std::mutex> lk(mutex);
    logical_size = size;
    buffer_bundle->resize(logical_size * scale);
}

void mc::BufferStreamSurfaces::force_requests_to_complete()
{
    buffer_bundle->force_requests_to_complete();
}

void mc::BufferStreamSurfaces::allow_framedropping(bool allow)
{
    buffer_bundle->allow_framedropping(allow);
}

int mc::BufferStreamSurfaces::buffers_ready_for_compositor(void const* user_id) const
{
    return buffer_bundle->buffers_ready_for_compositor(user_id);
}

void mc::BufferStreamSurfaces::drop_old_buffers()
{
    buffer_bundle->drop_old_buffers();
}

void mc::BufferStreamSurfaces::swap_buffers(
    mg::Buffer* old_buffer, std::function<void(mg::Buffer* new_buffer)> complete)
{
    if (old_buffer)
    {
        release_client_buffer(old_buffer);

        /*
         * TODO: In future frame_posted() could be made parameterless.
         *       The new method of catching up on buffer backlogs is to
         *       query buffers_ready_for_compositor() or Scene::frames_pending
         */
        observers.frame_posted(1);
    }

    acquire_client_buffer(complete);
}

bool mc::BufferStreamSurfaces::has_submitted_buffer() const
{
    std::unique_lock<std::mutex> lk(mutex);
    return first_frame_posted;
}

void mc::BufferStreamSurfaces::with_most_recent_buffer_do(std::function<void(graphics::Buffer&)> const& exec)
{
    if (!first_frame_posted)
        BOOST_THROW_EXCEPTION(std::runtime_error("No frame posted yet"));
    exec(*std::make_shared<mc::TemporarySnapshotBuffer>(buffer_bundle));
}

MirPixelFormat mc::BufferStreamSurfaces::pixel_format() const
{
    return buffer_bundle->properties().format;
}

void mc::BufferStreamSurfaces::add_observer(std::shared_ptr<scene::SurfaceObserver> const& observer)
{
    observers.add(observer);
}

void mc::BufferStreamSurfaces::remove_observer(std::weak_ptr<scene::SurfaceObserver> const& observer)
{
    if (auto o = observer.lock())
        observers.remove(o);
}

mg::BufferID mc::BufferStreamSurfaces::allocate_buffer(graphics::BufferProperties const&)
{
    BOOST_THROW_EXCEPTION(std::logic_error("buffer allocation cannot happen with an exchange-based buffer client"));
}

void mc::BufferStreamSurfaces::remove_buffer(graphics::BufferID)
{
    BOOST_THROW_EXCEPTION(std::logic_error("buffer removal cannot happen with an exchange-based buffer client"));
}

void mc::BufferStreamSurfaces::with_buffer(mg::BufferID, std::function<void(mg::Buffer&)> const&)
{
    BOOST_THROW_EXCEPTION(std::logic_error("buffer lookup cannot happen with an exchange-based buffer client"));
}

void mc::BufferStreamSurfaces::set_scale(float new_scale)
{
    if (new_scale <= 0.0f)
        BOOST_THROW_EXCEPTION(std::logic_error("invalid scale (must be greater than zero)"));

    std::unique_lock<std::mutex> lk(mutex);
    scale = new_scale;
    buffer_bundle->resize(logical_size * scale);
}
