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

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

mc::BufferStreamSurfaces::BufferStreamSurfaces(std::shared_ptr<BufferBundle> const& buffer_bundle)
    : buffer_bundle(buffer_bundle)
{
}

mc::BufferStreamSurfaces::~BufferStreamSurfaces()
{
    force_requests_to_complete();
}

std::shared_ptr<mg::Buffer> mc::BufferStreamSurfaces::lock_compositor_buffer(
    unsigned long frameno)
{
    return std::make_shared<mc::TemporaryCompositorBuffer>(
        buffer_bundle, frameno);
}

std::shared_ptr<mg::Buffer> mc::BufferStreamSurfaces::lock_snapshot_buffer()
{
    return std::make_shared<mc::TemporarySnapshotBuffer>(buffer_bundle);
}

std::shared_ptr<mg::Buffer> mc::BufferStreamSurfaces::secure_client_buffer()
{
    return std::make_shared<mc::TemporaryClientBuffer>(buffer_bundle);
}

geom::PixelFormat mc::BufferStreamSurfaces::get_stream_pixel_format()
{
    return buffer_bundle->properties().format; 
}

geom::Size mc::BufferStreamSurfaces::stream_size()
{
    return buffer_bundle->properties().size; 
}

void mc::BufferStreamSurfaces::resize(geom::Size const& size)
{
    buffer_bundle->resize(size);
}

void mc::BufferStreamSurfaces::force_requests_to_complete()
{
    buffer_bundle->force_requests_to_complete();
}

void mc::BufferStreamSurfaces::allow_framedropping(bool allow)
{
    buffer_bundle->allow_framedropping(allow);
}
