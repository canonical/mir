/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_bundle_surfaces.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/buffer_properties.h"

#include <cassert>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{
struct CompositorReleaseDeleter
{
    explicit CompositorReleaseDeleter(mc::BufferSwapper* sw, mc::BufferID id) :
        swapper(sw),
        id(id)
    {
    }

    void operator()(mc::GraphicBufferCompositorResource* compositor_resource)
    {
        swapper->compositor_release(id);
        delete compositor_resource;
    }

    mc::BufferSwapper* const swapper;
    mc::BufferID id;
};

struct ClientReleaseDeleter
{
    ClientReleaseDeleter(mc::BufferSwapper* sw) :
        swapper(sw)
    {
    }

    void operator()(mc::GraphicBufferClientResource* client_resource)
    {
        swapper->client_release(client_resource->id);
        delete client_resource;
    }

    mc::BufferSwapper* const swapper;
};
}

mc::BufferBundleSurfaces::BufferBundleSurfaces(
    std::unique_ptr<BufferSwapper>&& swapper,
    std::shared_ptr<BufferIDUniqueGenerator> gen, 
    BufferProperties const& buffer_properties)
     : generator(gen),
       swapper(std::move(swapper)),
       size(buffer_properties.size),
       pixel_format(buffer_properties.format)
{
}

mc::BufferBundleSurfaces::BufferBundleSurfaces(
    std::unique_ptr<BufferSwapper>&& swapper, std::shared_ptr<BufferIDUniqueGenerator> gen)
     : generator(gen),
       swapper(std::move(swapper)),
       size(),
       pixel_format(geometry::PixelFormat::rgba_8888)
{
}

mc::BufferBundleSurfaces::~BufferBundleSurfaces()
{
}

std::shared_ptr<mc::GraphicBufferCompositorResource> mc::BufferBundleSurfaces::lock_back_buffer()
{
    mc::BufferID id;
    std::weak_ptr<mc::Buffer> region;
    swapper->compositor_acquire(region, id);

    auto resource = new mc::GraphicBufferCompositorResource(region);
    CompositorReleaseDeleter del(swapper.get(), id);
    auto compositor_resource = std::shared_ptr<mc::GraphicBufferCompositorResource>(resource, del);

    return compositor_resource;
}

std::shared_ptr<mc::GraphicBufferClientResource> mc::BufferBundleSurfaces::secure_client_buffer()
{
    auto resource = new mc::GraphicBufferClientResource;
    swapper->client_acquire(resource->buffer, resource->id);
    ClientReleaseDeleter del(swapper.get());
    auto client_resource = std::shared_ptr<mc::GraphicBufferClientResource>(resource, del); 
    return client_resource;
}

geom::PixelFormat mc::BufferBundleSurfaces::get_bundle_pixel_format()
{
    return pixel_format;
}

geom::Size mc::BufferBundleSurfaces::bundle_size()
{
    return size;
}

void mc::BufferBundleSurfaces::shutdown()
{
    swapper->shutdown();
}
