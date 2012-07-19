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

#include <algorithm>
#include <mutex>

namespace mc = mir::compositor;

mc::BufferBundle::BufferBundle(std::unique_ptr<BufferSwapper>&& swapper)
 :
    swapper(std::move(swapper))
{
}

mc::BufferBundle::~BufferBundle()
{
}

void mc::BufferBundle::lock_back_buffer()
{
    auto compositor_buffer = swapper->grab_last_posted();
    compositor_buffer->lock();
}

void mc::BufferBundle::unlock_back_buffer()
{

}


std::shared_ptr<mc::Buffer> mc::BufferBundle::dequeue_client_buffer()
{
    auto client_buffer = swapper->dequeue_free_buffer();
    client_buffer->lock();

    struct NullDeleter { void operator()(void*) const {} };
    return std::shared_ptr<mc::Buffer>(client_buffer, NullDeleter());
}

void mc::BufferBundle::queue_client_buffer()
{
}
