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

#include "mir/compositor/buffer_stream_surfaces.h"
#include "buffer_bundle.h"
#include "mir/compositor/buffer_properties.h"

#include "temporary_buffers.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace ms = mir::surfaces;

mc::BufferStreamSurfaces::BufferStreamSurfaces(std::shared_ptr<BufferBundle> const& buffer_bundle)
     : buffer_bundle(buffer_bundle)
{
}

mc::BufferStreamSurfaces::~BufferStreamSurfaces()
{
}

std::shared_ptr<ms::GraphicRegion> mc::BufferStreamSurfaces::lock_back_buffer()
{
    return std::make_shared<mc::TemporaryCompositorBuffer>(buffer_bundle);
}

std::shared_ptr<mc::Buffer> mc::BufferStreamSurfaces::secure_client_buffer()
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

void mc::BufferStreamSurfaces::shutdown()
{
    buffer_bundle->shutdown();
}
