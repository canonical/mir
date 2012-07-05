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
#include <mir/compositor/buffer_bundle.h>

#include <mutex>

namespace mc = mir::compositor;

mc::BufferBundle::BufferBundle()
{
}

mc::BufferBundle::~BufferBundle()
{
}

void mc::BufferBundle::add_buffer(std::shared_ptr<Buffer> buffer)
{

    std::lock_guard<std::mutex> lg(buffer_list_guard);
    std::lock_guard<std::mutex> lg_back_buffer(back_buffer_guard);
    compositor_buffer = client_buffer;
    client_buffer = buffer;

    buffer_list.push_back(buffer);     
}

int mc::BufferBundle::remove_all_buffers()
{
    std::lock_guard<std::mutex> lg(buffer_list_guard);

    int size = buffer_list.size();
    buffer_list.clear();

    return size;
}

void mc::BufferBundle::lock_back_buffer()
{
    compositor_buffer->lock();
}

void mc::BufferBundle::unlock_back_buffer()
{
    compositor_buffer->unlock();
}

std::shared_ptr<mc::Buffer> mc::BufferBundle::back_buffer()
{
    return compositor_buffer;
}

void mc::BufferBundle::queue_client_buffer(std::shared_ptr<mc::Buffer> buffer)
{
    // TODO: This is a very dumb strategy for locking
    std::lock_guard<mc::Buffer> lg_compositor_buffer(*compositor_buffer);
    std::lock_guard<mc::Buffer> lg_client_buffer(*client_buffer);

    std::swap(compositor_buffer, client_buffer);
    std::swap(compositor_buffer, buffer);
}

std::shared_ptr<mc::Buffer> mc::BufferBundle::dequeue_client_buffer()
{
    client_buffer->lock();
    return client_buffer;
}


