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
#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/graphics/texture.h"

#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{
class TexDeleter
{

public:
    TexDeleter(mc::BufferSwapper* sw, mc::Buffer* buf)
        : swapper(std::move(sw)),
          buffer_ptr(buf)
    {
    };

    void operator()(mg::Texture* texture)
    {
        swapper->compositor_release(buffer_ptr);
        delete texture;
    }

private:
    mc::BufferSwapper* const swapper;
    mc::Buffer* const buffer_ptr;
};


class BufDeleter
{
public:
    BufDeleter(mc::BufferSwapper* sw, mc::Buffer* buf)
        : swapper(sw),
          buffer_ptr(buf)
    {
    };

    void operator()(mc::BufferIPCPackage* package)
    {
        swapper->client_release(buffer_ptr);
        delete package;
    }

private:
    mc::BufferSwapper* const swapper;
    mc::Buffer* const buffer_ptr;
};
}

mc::BufferBundle::BufferBundle(std::unique_ptr<BufferSwapper>&& swapper)
    :
    swapper(std::move(swapper))
{
}

mc::BufferBundle::~BufferBundle()
{
}

std::shared_ptr<mir::graphics::Texture> mc::BufferBundle::lock_and_bind_back_buffer()
{
    auto compositor_buffer = swapper->compositor_acquire();
    compositor_buffer->bind_to_texture();

    mg::Texture* tex = new mg::Texture;
    TexDeleter deleter(swapper.get(), compositor_buffer);
    return std::shared_ptr<mg::Texture>(tex, deleter);
}

struct EmptyDeleter
{
    template<typename T>
    void operator()(T* )
    {
    }
};
std::shared_ptr<mc::BufferResource> mc::BufferBundle::secure_client_buffer()
{
//    auto client_buffer = swapper->client_acquire();

    /* todo: splicing ownership is bad */
//    mc::BufferIPCPackage* buf = client_buffer->get_ipc_package().get();
//    BufDeleter deleter(swapper.get(), client_buffer);
//    return std::shared_ptr<mc::BufferIPCPackage>(buf, deleter);
    BufferResource* tmp = NULL;
    EmptyDeleter del;
    std::shared_ptr<mc::BufferResource> ret(tmp, del);
    return ret;
}

std::shared_ptr<mc::BufferIPCPackage> mc::BufferBundle::get_ipc_package(std::shared_ptr<BufferResource>)
{

    BufferIPCPackage* tmp = NULL;
    EmptyDeleter del;
    std::shared_ptr<mc::BufferIPCPackage> ret(tmp, del);
    return ret;
} 
