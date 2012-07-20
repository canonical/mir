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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/buffer_swapper_double.h"
#include "mir/compositor/buffer.h"

#include <iostream>
namespace mc = mir::compositor;

mc::BufferSwapperDouble::BufferSwapperDouble(std::unique_ptr<Buffer> && buf_a, std::unique_ptr<Buffer> && buf_b)
    :
    grabbed_buffer(nullptr),
    buffer_a(std::move(buf_a)),
    buffer_b(std::move(buf_b))
{
    client_queue.push(buffer_a.get());
    client_queue.push(buffer_b.get());
}

mc::Buffer* mc::BufferSwapperDouble::dequeue_free_buffer()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);
    
    while (client_queue.empty())
    {
        available_cv.wait(lk);
    }

    Buffer* dequeued_buffer = client_queue.front(); 
    client_queue.pop();
    return dequeued_buffer;
}

void mc::BufferSwapperDouble::queue_finished_buffer(mc::Buffer* queued_buffer)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);
    while (grabbed_buffer != nullptr)
    {
        posted_cv.wait(lk);
    }

    grabbed_buffer = queued_buffer;  
}

mc::Buffer* mc::BufferSwapperDouble::grab_last_posted()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);


    grabbed_buffer = nullptr;
    return nullptr;
}

void mc::BufferSwapperDouble::ungrab()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

}

