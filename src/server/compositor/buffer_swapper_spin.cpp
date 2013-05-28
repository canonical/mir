/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/buffer_swapper_spin.h"

namespace mc = mir::compositor;

std::shared_ptr<mc::Buffer> mc::BufferSwapperSpin::client_acquire()
{
    std::lock_guard<std::mutex> lg{swapper_mutex};

    /*
     * Don't allow the client to acquire all the buffers, because then the
     * compositor won't have a buffer to display.
     */
    if (in_use_by_client == swapper_size - 1)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Client is trying to acquire all buffers at once"));
    }

    auto dequeued_buffer = buffer_queue.back();
    buffer_queue.pop_back();
    in_use_by_client++;

    return dequeued_buffer;
}

void mc::BufferSwapperSpin::client_release(std::shared_ptr<Buffer> const& queued_buffer)
{
    std::lock_guard<std::mutex> lg{swapper_mutex};

    buffer_queue.push_front(queued_buffer);
    in_use_by_client--;
    client_submitted_new_buffer = true;
}

std::shared_ptr<mc::Buffer> mc::BufferSwapperSpin::compositor_acquire()
{
    std::lock_guard<std::mutex> lg{swapper_mutex};

    auto dequeued_buffer = buffer_queue.front();
    buffer_queue.pop_front();
    client_submitted_new_buffer = false;

    return dequeued_buffer;
}

void mc::BufferSwapperSpin::compositor_release(std::shared_ptr<Buffer> const& released_buffer)
{
    std::lock_guard<std::mutex> lg{swapper_mutex};

    /*
     * If the client didn't submit a new buffer while the compositor was
     * holding a buffer, the compositor's buffer is still the newest one, so
     * place it at the front of the buffer queue. Otherwise, place it at the
     * back of the buffer queue.
     */
    if (client_submitted_new_buffer)
        buffer_queue.push_back(released_buffer);
    else
        buffer_queue.push_front(released_buffer);
}

void mc::BufferSwapperSpin::force_requests_to_complete()
{
}
