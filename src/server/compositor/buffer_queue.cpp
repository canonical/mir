/*
 * Copyright © 2014 Canonical Ltd.
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
 */

#include "buffer_queue.h"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_id.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{
bool remove(std::shared_ptr<mg::Buffer> const& buffer,
            std::vector<std::shared_ptr<mg::Buffer>>& list)
{
    auto const& begin = list.begin();
    auto const& end = list.end();
    auto it = std::find(begin, end, buffer);

    /* nothing to remove */
    if (it == end)
        return false;

    list.erase(it);
    return true;
}

std::shared_ptr<mg::Buffer> pop(std::deque<std::shared_ptr<mg::Buffer>>& q)
{
    auto buffer = q.front();
    q.pop_front();
    return buffer;
}

bool contains(std::shared_ptr<mg::Buffer> const& buffer,
              std::vector<std::shared_ptr<mg::Buffer>> const& list)
{
    if (list.empty())
        return false;

    auto const& begin = list.cbegin();
    auto const& end = list.cend();
    return std::find(begin, end, buffer) != end;
}
}

mc::BufferQueue::BufferQueue(
    int nbuffers,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& gralloc,
    graphics::BufferProperties const& props)
    : nbuffers{nbuffers},
      allocated_buffers{std::min(nbuffers, 2)},
      frame_dropping_enabled{false},
      the_properties{props},
      gralloc{gralloc}
{
    if (nbuffers < 1)
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("invalid number of buffers for BufferQueue"));
    }

    /* By default not all buffers are allocated.
     * If there is increased pressure by the client to acquire
     * more buffers, more will be allocated at that time (up to nbuffers)
     */
    for(int i = 0; i < allocated_buffers - 1; i++)
    {
        free_buffers.push_back(gralloc->alloc_buffer(the_properties));
    }
    current_compositor_buffer = gralloc->alloc_buffer(the_properties);

    if (nbuffers == 1)
        free_buffers.push_back(current_compositor_buffer);
}

void mc::BufferQueue::client_acquire(std::function<void(graphics::Buffer* buffer)> complete)
{
    std::unique_lock<decltype(guard)> lock(guard);

    if (give_to_client)
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("unexpected acquire: there is a pending completion"));
    }

    give_to_client = std::move(complete);

    if (!free_buffers.empty())
    {
        auto buffer = free_buffers.back();
        free_buffers.pop_back();
        give_buffer_to_client(buffer, std::move(lock));
        return;
    }

    /* No empty buffers, attempt allocating more
     * TODO: need a good heuristic to switch
     * between double-buffering to n-buffering
	 */
    if (allocated_buffers < nbuffers)
    {
        auto const& buffer = gralloc->alloc_buffer(the_properties);
        allocated_buffers++;
        give_buffer_to_client(buffer, std::move(lock));
        return;
    }

    /* Last resort, drop oldest buffer from the ready queue */
    if (frame_dropping_enabled && !ready_to_composite_queue.empty())
    {
        auto const& buffer = pop(ready_to_composite_queue);
        give_buffer_to_client(buffer, std::move(lock));
    }
}

void mc::BufferQueue::client_release(graphics::Buffer* released_buffer)
{
    std::lock_guard<decltype(guard)> lock(guard);

    if (buffers_owned_by_client.empty())
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("unexpected release: no buffers were given to client"));
    }

    if (buffers_owned_by_client.front().get() != released_buffer)
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("client released out of sequence"));
    }

    auto const& buffer = pop(buffers_owned_by_client);
    ready_to_composite_queue.push_back(buffer);
}

std::shared_ptr<mg::Buffer>
mc::BufferQueue::compositor_acquire(void const* user_id)
{
    std::unique_lock<decltype(guard)> lock(guard);

    bool use_current_buffer =
        is_new_user(user_id) || ready_to_composite_queue.empty();

    std::shared_ptr<mg::Buffer> buffer_to_release;
    if (!use_current_buffer)
    {
        /* No other compositors currently reference this
         * buffer so release it
         */
        if (buffers_sent_to_compositor.empty())
            buffer_to_release = current_compositor_buffer;

        /* The current compositor buffer is
         * being changed, the new one has no users yet
         */
        compositor_ids.clear();
        current_compositor_buffer = pop(ready_to_composite_queue);
    }

    buffers_sent_to_compositor.push_back(current_compositor_buffer);
    compositor_ids.push_back(user_id);

    if (buffer_to_release)
        release(buffer_to_release, std::move(lock));

    return current_compositor_buffer;
}

void mc::BufferQueue::compositor_release(std::shared_ptr<graphics::Buffer> const& buffer)
{
    std::unique_lock<decltype(guard)> lock(guard);

    if (!remove(buffer, buffers_sent_to_compositor))
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("unexpected release: buffer was not given to compositor"));
    }

    /* Not ready to release it yet, other compositors still reference this buffer */
    if (contains(buffer, buffers_sent_to_compositor))
        return;

    /* This buffer is not owned by compositor anymore */
    if (current_compositor_buffer != buffer)
        release(buffer, std::move(lock));
}

std::shared_ptr<mg::Buffer> mc::BufferQueue::snapshot_acquire()
{
    std::unique_lock<decltype(guard)> lock(guard);
    pending_snapshots.push_back(current_compositor_buffer);
    return current_compositor_buffer;
}

void mc::BufferQueue::snapshot_release(std::shared_ptr<graphics::Buffer> const& buffer)
{
    std::unique_lock<std::mutex> lock(guard);
    if (!remove(buffer, pending_snapshots))
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("unexpected release: no buffers were given to snapshotter"));
    }

    snapshot_released.notify_all();
}

mg::BufferProperties mc::BufferQueue::properties() const
{
    std::lock_guard<decltype(guard)> lock(guard);
    return the_properties;
}

void mc::BufferQueue::allow_framedropping(bool flag)
{
    std::lock_guard<decltype(guard)> lock(guard);
    frame_dropping_enabled = flag;
}

bool mc::BufferQueue::framedropping_allowed() const
{
    std::lock_guard<decltype(guard)> lock(guard);
    return frame_dropping_enabled;
}

void mc::BufferQueue::force_requests_to_complete()
{
    std::unique_lock<std::mutex> lock(guard);
    if (give_to_client && !ready_to_composite_queue.empty())
    {
        auto const& buffer = pop(ready_to_composite_queue);
        while (!ready_to_composite_queue.empty())
        {
            free_buffers.push_back(pop(ready_to_composite_queue));
        }
        give_buffer_to_client(buffer, std::move(lock));
    }
}

void mc::BufferQueue::resize(geometry::Size const& new_size)
{
    std::lock_guard<decltype(guard)> lock(guard);
    the_properties.size = new_size;
}

int mc::BufferQueue::buffers_ready_for_compositor() const
{
    std::lock_guard<decltype(guard)> lock(guard);

    /*TODO: this api also needs to know the caller user id
     * as the number of buffers that are truly ready
     * vary depending on concurrent compositors.
     */
    return ready_to_composite_queue.size();
}

void mc::BufferQueue::give_buffer_to_client(
    std::shared_ptr<mg::Buffer> const& buffer_for_client,
    std::unique_lock<std::mutex> lock)
{
    /* Clears callback */
    auto give_to_client_cb = std::move(give_to_client);

    std::shared_ptr<mg::Buffer> buffer{buffer_for_client};

    bool new_buffer = buffer->size() != the_properties.size;
    if (new_buffer)
        buffer = gralloc->alloc_buffer(the_properties);

    /* Don't give to the client just yet if there's a pending snapshot */
    if (!new_buffer && contains(buffer, pending_snapshots))
    {
        snapshot_released.wait(lock,
            [&]{ return !contains(buffer, pending_snapshots); });
    }

    buffers_owned_by_client.push_back(buffer);

    lock.unlock();
    try
    {
        give_to_client_cb(buffer.get());
    }
    catch (...)
    {
        /* comms errors should not propagate to compositing threads */
    }
}

bool mc::BufferQueue::is_new_user(void const* user_id)
{
    if (!compositor_ids.empty())
    {
        auto const& begin = compositor_ids.cbegin();
        auto const& end = compositor_ids.cend();
        return std::find(begin, end, user_id) == end;
    }
    /* The id list is really only empty the very first time.
     * False is returned so that the compositor buffer is advanced.
     */
    return false;
}

void mc::BufferQueue::release(
    std::shared_ptr<mg::Buffer> const& buffer,
    std::unique_lock<std::mutex> lock)
{
    if (give_to_client)
        give_buffer_to_client(buffer, std::move(lock));
    else
        free_buffers.push_back(buffer);
}
