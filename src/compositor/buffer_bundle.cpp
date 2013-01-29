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

#include "mir/compositor/temporary_buffer.h"
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
    CompositorReleaseDeleter(std::shared_ptr<mc::BufferSwapper> sw, mc::BufferID id) :
        swapper(sw),
        id(id)
    {
    }

    void operator()(mc::GraphicRegion* compositor_resource)
    {
        if (auto res = swapper.lock())
            res->compositor_release(id);
        delete compositor_resource;
    }

    std::weak_ptr<mc::BufferSwapper> swapper;
    mc::BufferID id;
};

struct ClientReleaseDeleter
{
    ClientReleaseDeleter(std::shared_ptr<mc::BufferSwapper> sw, mc::BufferID id) :
        swapper(sw),
        id(id)
    {
    }

    void operator()(mc::Buffer* client_resource)
    {
        if (auto res = swapper.lock())
            res->client_release(id);
        delete client_resource;
    }

    std::weak_ptr<mc::BufferSwapper> swapper;
    mc::BufferID id;
};
}

mc::BufferBundleSurfaces::BufferBundleSurfaces(
    std::unique_ptr<BufferSwapper>&& swapper,
    BufferProperties const& buffer_properties)
     : swapper(std::move(swapper)),
       size(buffer_properties.size),
       pixel_format(buffer_properties.format)
{
}

mc::BufferBundleSurfaces::BufferBundleSurfaces(std::unique_ptr<BufferSwapper>&& swapper)
     : swapper(std::move(swapper)),
       size(),
       pixel_format(geometry::PixelFormat::abgr_8888)
{
}

mc::BufferBundleSurfaces::~BufferBundleSurfaces()
{
}

std::shared_ptr<mc::GraphicRegion> mc::BufferBundleSurfaces::lock_back_buffer()
{
    mc::BufferID id;
    std::shared_ptr<mc::Buffer> region;
    swapper->compositor_acquire(region, id);

    std::function<void()> func;
    auto resource = new mc::TemporaryBuffer(region, func);
    CompositorReleaseDeleter del(swapper, id);
    auto compositor_resource = std::shared_ptr<mc::Buffer>(resource, del);

    return compositor_resource;
}

std::shared_ptr<mc::Buffer> mc::BufferBundleSurfaces::secure_client_buffer()
{
    BufferID id;
    std::shared_ptr<Buffer> buffer;
    swapper->client_acquire(buffer, id);

    std::function<void()> func;
    auto resource = new mc::TemporaryBuffer(buffer, func);
    ClientReleaseDeleter del(swapper, id);
    auto client_resource = std::shared_ptr<mc::Buffer>(resource, del);

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
