/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "fb_swapper.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace mc=mir::compositor;

template<class T>
void mga::FBSwapper::initialize_queues(T buffer_list)
{
    for (auto& buffer : buffer_list)
    {
        queue.push(buffer);
    }
}

mga::FBSwapper::FBSwapper(std::initializer_list<std::shared_ptr<mc::Buffer>> buffer_list)
{
    initialize_queues(buffer_list);
}

mga::FBSwapper::FBSwapper(std::vector<std::shared_ptr<mc::Buffer>> buffer_list)
{
    initialize_queues(buffer_list);
}

std::shared_ptr<mc::Buffer> mga::FBSwapper::compositor_acquire()
{
    std::unique_lock<std::mutex> lk(queue_lock);
    while (queue.empty())
    {
        cv.wait(lk);
    }

    auto buffer = queue.front();
    queue.pop();
    return buffer;
}

void mga::FBSwapper::compositor_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lk(queue_lock);

    queue.push(released_buffer);
    cv.notify_all();
}

void mga::FBSwapper::shutdown()
{
}

/* the client_{acquire,release} functions are how we will hand out framebuffers to the clients.
 * and facilitate the handout between an internal composition renderloop and one that is in the client.
 * they are unsupported at this time */
void mga::FBSwapper::composition_bypass_unsupported()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("composition bypass is unsupported"));
}
std::shared_ptr<mc::Buffer> mga::FBSwapper::client_acquire()
{
    composition_bypass_unsupported();
    return std::shared_ptr<mc::Buffer>(); 
}

void mga::FBSwapper::client_release(std::shared_ptr<mc::Buffer> const& /*queued_buffer*/)
{
    composition_bypass_unsupported();
}

