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
    buffer_b(std::move(buf_b)),
    consumed(true)
{
    client_queue.push(buffer_a.get());
    client_queue.push(buffer_b.get());
}

mc::Buffer* mc::BufferSwapperDouble::dequeue_free_buffer()
{
    std::cout << "DQ\n";
    std::unique_lock<std::mutex> lk(swapper_mutex);
 
    while (client_queue.empty())
    {
        std::cout << "WAIT ON DQ\n";
        available_cv.wait(lk);
    }

    Buffer* dequeued_buffer = client_queue.front(); 
    client_queue.pop();
    std::cout << "return DQ\n";
    return dequeued_buffer;
}

void mc::BufferSwapperDouble::queue_finished_buffer(mc::Buffer* queued_buffer)
{
    std::cout << "Q\n";
    std::unique_lock<std::mutex> lk(swapper_mutex);

    while (!consumed)
    {
        std::cout << "WAIT ON Q\n";
        posted_cv.wait(lk);
    }
    consumed = false;

    if(grabbed_buffer != nullptr)
    {
        client_queue.push(grabbed_buffer);
    }

    grabbed_buffer = queued_buffer;
     
}

mc::Buffer* mc::BufferSwapperDouble::grab_last_posted()
{
    std::cout << "G\n";
    std::unique_lock<std::mutex> lk(swapper_mutex);

    mc::Buffer* last_posted;
    last_posted = grabbed_buffer;

    grabbed_buffer = nullptr;

    consumed = true;
    posted_cv.notify_one();
    return last_posted;
}

void mc::BufferSwapperDouble::ungrab(mc::Buffer *ungrabbed_buffer)
{
    std::cout << "UG\n";
    std::unique_lock<std::mutex> lk(swapper_mutex);
    if (grabbed_buffer == nullptr)
    {
        grabbed_buffer = ungrabbed_buffer;
    }
    else
    {
        client_queue.push(ungrabbed_buffer);
        available_cv.notify_one();
    }
    std::cout << "UG exit\n";
}

