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

    auto release_function = [] (std::weak_ptr<mc::BufferSwapper> s, mc::BufferID release_id)
    {
        if (auto swap = s.lock())
            swap->compositor_release(release_id);
    };
    auto func2 = std::bind(release_function, swapper, id); 
    auto compositor_resource = std::make_shared<mc::TemporaryBuffer>(region, func2);

    return compositor_resource;
}

std::shared_ptr<mc::Buffer> mc::BufferBundleSurfaces::secure_client_buffer()
{
    BufferID id;
    std::shared_ptr<Buffer> buffer;
    swapper->client_acquire(buffer, id);

    auto release_function = [] (std::weak_ptr<mc::BufferSwapper> s, mc::BufferID release_id)
    {
        if (auto swap = s.lock())
            swap->client_release(release_id);
    };
    auto func2 = std::bind(release_function, swapper, id); 
    auto client_resource = std::make_shared<mc::TemporaryBuffer>(buffer, func2);

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
