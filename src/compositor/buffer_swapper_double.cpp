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

namespace mc = mir::compositor;

mc::BufferSwapperDouble::BufferSwapperDouble(std::unique_ptr<Buffer> && buf_a, std::unique_ptr<Buffer> && buf_b)
    :
    buffer_a(std::move(buf_a)),
    buffer_b(std::move(buf_b)),
    compositor_has_consumed(true)
{
    client_queue.push(buffer_a.get());
    last_posted_buffer = buffer_b.get();
}


mc::Buffer* mc::BufferSwapperDouble::client_acquire()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    while (client_queue.empty())
    {
        buffer_available_cv.wait(lk);
    }

    Buffer* dequeued_buffer = client_queue.front();
    client_queue.pop();

    return dequeued_buffer;
}

void mc::BufferSwapperDouble::client_release(mc::Buffer* queued_buffer)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    while (!compositor_has_consumed)
    {
        consumed_cv.wait(lk);
    }
    compositor_has_consumed = false;

    if(last_posted_buffer != NULL)
    {
        client_queue.push(last_posted_buffer);
        buffer_available_cv.notify_one();
    }

    last_posted_buffer = queued_buffer;

}

mc::Buffer* mc::BufferSwapperDouble::compositor_acquire()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    mc::Buffer* last_posted;
    last_posted = last_posted_buffer;
    last_posted_buffer = NULL;

    compositor_has_consumed = true;
    consumed_cv.notify_one();
    return last_posted;
}

void mc::BufferSwapperDouble::compositor_release(mc::Buffer *released_buffer)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);
    if (last_posted_buffer == NULL)
    {
        last_posted_buffer = released_buffer;
    }
    else
    {
        client_queue.push(released_buffer);
        buffer_available_cv.notify_one();
    }
}
