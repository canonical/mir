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
#include "mir/graphics/texture.h"

#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
struct CompositorReleaseDeleter
{
    explicit CompositorReleaseDeleter(mc::BufferSwapper* sw) :
        swapper(sw)
    {
    }

    void operator()(mc::Buffer* buffer)
    {
        swapper->compositor_release(buffer);
    }

    mc::BufferSwapper* const swapper;
};

struct ClientReleaseDeleter
{
    ClientReleaseDeleter(mc::BufferSwapper* sw) :
        swapper(sw)
    {
    }

    void operator()(mc::Buffer* buffer)
    {
        swapper->client_release(buffer);
    }

    mc::BufferSwapper* const swapper;
};
}

mc::BufferBundleSurfaces::BufferBundleSurfaces(std::unique_ptr<BufferSwapper>&& swapper)
    :
    swapper(std::move(swapper))
{
}

mc::BufferBundleSurfaces::~BufferBundleSurfaces()
{
}

std::shared_ptr<mir::graphics::Texture> mc::BufferBundleSurfaces::lock_and_bind_back_buffer()
{
    std::shared_ptr<Buffer> bptr{swapper->compositor_acquire(), CompositorReleaseDeleter(swapper.get())};
    bptr->bind_to_texture();

    return std::make_shared<mg::Texture>(bptr);
}

std::shared_ptr<mc::GraphicBufferClientResource> mc::BufferBundleSurfaces::secure_client_buffer()
{
    std::shared_ptr<Buffer> bptr{swapper->client_acquire(), ClientReleaseDeleter(swapper.get())};

    // TODO This is a MASSIVE frig to get tests passing
    if (!bptr.get()) // StubGraphicBufferAllocator leaves this null
    {
        return std::make_shared<mc::GraphicBufferClientResource>(
            std::make_shared<BufferIPCPackage>(), bptr);
    }

    return std::make_shared<mc::GraphicBufferClientResource>(bptr->get_ipc_package(), bptr);
}

mc::PixelFormat mc::BufferBundleSurfaces::get_bundle_pixel_format()
{
    return mc::PixelFormat::rgba_5658;
}

geom::Height mc::BufferBundleSurfaces::bundle_height()
{
    return geom::Height(0);
}

geom::Width mc::BufferBundleSurfaces::bundle_width()
{
    return geom::Width(0);
}
