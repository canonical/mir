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
    compositor_has_consumed(true),
    client_queue(std::move(buf_a)),
    last_posted_buffer(std::move(buf_b))
{
}

std::shared_ptr<mc::Buffer> mc::BufferSwapperDouble::client_acquire()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    while (client_queue.get() == NULL)
    {
        buffer_available_cv.wait(lk);
    }

    return std::move(client_queue);
}

void mc::BufferSwapperDouble::client_release(std::shared_ptr<mc::Buffer> queued_buffer)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    while (!compositor_has_consumed)
    {
        consumed_cv.wait(lk);
    }
    compositor_has_consumed = false;

    if(last_posted_buffer != NULL)
    {
        client_queue = std::move(last_posted_buffer);
        buffer_available_cv.notify_one();
    }

    last_posted_buffer = std::move(queued_buffer);

}

std::shared_ptr<mc::Buffer> mc::BufferSwapperDouble::compositor_acquire()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    std::shared_ptr<mc::Buffer> last_posted;
    last_posted = std::move(last_posted_buffer);

    compositor_has_consumed = true;
    consumed_cv.notify_one();

    return last_posted;
}

void mc::BufferSwapperDouble::compositor_release(std::shared_ptr<mc::Buffer> released_buffer)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);
    if (last_posted_buffer == NULL)
    {
        last_posted_buffer = std::move(released_buffer);
    }
    else
    {
        client_queue = std::move(released_buffer);
        buffer_available_cv.notify_one();
    }
}
