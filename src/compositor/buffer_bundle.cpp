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
#include <cassert>
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
    compositor_buffer = swapper->compositor_secure_last_posted();
    compositor_buffer->lock();
}

void mc::BufferBundle::unlock_back_buffer()
{
    compositor_buffer->unlock();
//    swapper->compositor_release();
}

std::shared_ptr<mc::Buffer> mc::BufferBundle::back_buffer()
{
    struct NullDeleter { void operator()(void*) const {} };
    return std::shared_ptr<mc::Buffer>(compositor_buffer, NullDeleter());
}

void mc::BufferBundle::queue_client_buffer(std::shared_ptr<mc::Buffer> buffer)
{
    assert(client_buffer == buffer.get());

    client_buffer->unlock();
//    swapper->client_release_finished_buffer();
}

std::shared_ptr<mc::Buffer> mc::BufferBundle::dequeue_client_buffer()
{
    client_buffer = swapper->client_acquire_buffer();
    client_buffer->lock();
    struct NullDeleter { void operator()(void*) const {} };
    return std::shared_ptr<mc::Buffer>(client_buffer, NullDeleter());
}


