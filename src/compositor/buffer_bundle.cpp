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

void mc::BufferBundle::bind_back_buffer()
{
    std::lock_guard<std::mutex> lg(buffer_list_guard);

    compositor_buffer->lock();
    compositor_buffer->bind_to_texture();

}

void mc::BufferBundle::release_back_buffer() {


}

void mc::BufferBundle::queue_client_buffer(std::shared_ptr<mc::Buffer>)
{

}

std::shared_ptr<mc::Buffer> mc::BufferBundle::dequeue_client_buffer()
{
    std::lock_guard<std::mutex> lg(buffer_list_guard);
    client_buffer->lock();
    return nullptr;
}


