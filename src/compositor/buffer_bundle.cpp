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
#include "mir/thread/all.h"

namespace mc = mir::compositor;
namespace mg = mir::graphics;

mc::BufferBundle::BufferBundle(std::unique_ptr<BufferSwapper>&& swapper)
 :
    swapper(std::move(swapper))
{
}

mc::BufferBundle::~BufferBundle()
{
}

namespace
{
struct NullDeleter { void operator()(void*) const {} };
}
std::shared_ptr<mir::graphics::Texture> mc::BufferBundle::lock_and_bind_back_buffer()
{
    auto compositor_buffer = swapper->compositor_acquire();
    compositor_buffer->bind_to_texture();

    mg::Texture* tex = NULL;
    return std::shared_ptr<mg::Texture>(tex, NullDeleter());
}

void mc::BufferBundle::unlock_back_buffer()
{
    swapper->compositor_release(NULL);
}

void mc::BufferBundle::queue_client_buffer(std::shared_ptr<mc::Buffer> buffer)
{
    auto client_buffer = buffer.get();
    assert(client_buffer);

    swapper->client_release(client_buffer);
}

std::shared_ptr<mc::Buffer> mc::BufferBundle::dequeue_client_buffer()
{
    mc::Buffer* client_buffer = swapper->client_acquire();
    client_buffer->lock();

    return std::shared_ptr<mc::Buffer>(client_buffer, NullDeleter());
}
