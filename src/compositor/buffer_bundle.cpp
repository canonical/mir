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
#include "mir/graphics/texture.h"

#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::BufferBundle::BufferBundle(std::unique_ptr<BufferSwapper> swapper)
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
    mc::TexDeleter deleter(swapper.get(), compositor_buffer);
    return std::shared_ptr<mg::Texture>(tex, deleter);
}

std::shared_ptr<mc::BufferIPCPackage> mc::BufferBundle::secure_client_buffer()
{
    auto client_buffer = swapper->client_acquire();
    client_buffer->lock();

    mc::BufferIPCPackage* buf = new mc::BufferIPCPackage;
    mc::BufDeleter deleter(swapper.get(), client_buffer);
    return std::shared_ptr<mc::BufferIPCPackage>(buf, deleter);
}
