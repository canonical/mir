/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/buffer_swapper_multi.h"
#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;

template<class T>
void mc::BufferSwapperMulti::initialize_queues(T buffer_list)
{
    if ((buffer_list.size() == 1) || (buffer_list.size() >= 4))
    {
        BOOST_THROW_EXCEPTION(std::logic_error("BufferSwapperMulti is only validated for 2 or 3 buffers"));
    }

    for (auto& buffer : buffer_list)
    {
        client_queue.push_back(buffer);
    }
}

mc::BufferSwapperMulti::BufferSwapperMulti(std::vector<std::shared_ptr<compositor::Buffer>> buffer_list)
 : in_use_by_client(0),
   swapper_size(buffer_list.size()),
   force_clients_to_complete(false)
{
    initialize_queues(buffer_list);
}

mc::BufferSwapperMulti::BufferSwapperMulti(std::initializer_list<std::shared_ptr<compositor::Buffer>> buffer_list) :
    in_use_by_client(0),
    swapper_size(buffer_list.size()),
    force_clients_to_complete(false)
{
    initialize_queues(buffer_list);
}

std::shared_ptr<mc::Buffer> mc::BufferSwapperMulti::client_acquire()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    /*
     * Don't allow the client to acquire all the buffers, because then the
     * compositor won't have a buffer to display.
     */
    while ((swapper_size > 0) &&
           (client_queue.empty() || (in_use_by_client == (swapper_size - 1))))
    {
        client_available_cv.wait(lk);
    }

    //we have been forced to shutdown
    if (swapper_size == 0)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("forced_completion"));
    }

    auto dequeued_buffer = client_queue.front();
    client_queue.pop_front();
    in_use_by_client++;

    return dequeued_buffer;
}

void mc::BufferSwapperMulti::client_release(std::shared_ptr<Buffer> const& queued_buffer)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    compositor_queue.push_back(queued_buffer);
    in_use_by_client--;

    /*
     * At this point we could signal the client_available_cv.  However, we
     * won't do so, because the cv will get signaled after the next
     * compositor_release anyway, and we want to avoid the overhead of
     * signaling at every client_release just to ensure quicker client
     * notification in an abnormal situation.
     */
}

std::shared_ptr<mc::Buffer> mc::BufferSwapperMulti::compositor_acquire()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    if (compositor_queue.empty() && client_queue.empty())
    {
        BOOST_THROW_EXCEPTION(std::logic_error("forced_completion"));
    }

    std::shared_ptr<mc::Buffer> dequeued_buffer;
    if (compositor_queue.empty())
    {
        dequeued_buffer = client_queue.back();
        client_queue.pop_back();
    }
    else
    {
        dequeued_buffer = compositor_queue.front();
        compositor_queue.pop_front();
    }

    return dequeued_buffer;
}

void mc::BufferSwapperMulti::compositor_release(std::shared_ptr<Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);
    client_queue.push_back(released_buffer);
    client_available_cv.notify_one();
}

void mc::BufferSwapperMulti::force_client_completion()
{
    std::unique_lock<std::mutex> lk(swapper_mutex);
    swapper_size = 0;
    client_available_cv.notify_all();
}

void mc::BufferSwapperMulti::end_responsibility(std::vector<std::shared_ptr<Buffer>>& buffers,
                                                size_t& size)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    while(!compositor_queue.empty())
    {
        buffers.push_back(compositor_queue.back());
        compositor_queue.pop_back();
    }

    while(!client_queue.empty())
    {
        buffers.push_back(client_queue.back());
        client_queue.pop_back();
    }

    size = swapper_size;
}
#if 0
void mc::BufferSwapperMulti::transfer_buffers_to(std::shared_ptr<BufferSwapper> const& new_swapper)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);

    while(!compositor_queue.empty())
    {
        new_swapper->adopt_buffer(compositor_queue.back());
        compositor_queue.pop_back();
    }

    while(!client_queue.empty())
    {
        new_swapper->adopt_buffer(client_queue.back());
        client_queue.pop_back();
    }
}

void mc::BufferSwapperMulti::adopt_buffer(std::shared_ptr<Buffer> const& buffer)
{
    std::unique_lock<std::mutex> lk(swapper_mutex);
    client_queue.push_back(buffer);
    //swapper_size++;
    swapper_size = 3;

    client_available_cv.notify_one();
}
#endif
