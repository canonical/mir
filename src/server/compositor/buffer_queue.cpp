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
    for (unsigned int i = 0; i < list.size(); i++)
    {
        if (buffer == list[i])
        {
            list.erase(list.begin() + i);
            return true;
        }
    }
    /* buffer was not in the list */
    return false;
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
    /* Avoid allocating iterators, the list is small enough
     * that the overhead of iterator creation is greater than the
     * search itself
     */
    for (unsigned int i = 0; i < list.size(); i++)
    {
        if (buffer == list[i])
            return true;
    }
    return false;
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
    if (nbuffers < min_num_buffers)
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("invalid number of buffers for BufferQueue"));
    }

    /* By default not all buffers are allocated.
     * If there is increased pressure by the client to acquire
     * more buffers, more will be allocated at that time (up to nbuffers)
     */
    for(int i = 0; i < allocated_buffers; i++)
    {
        free_queue.push_back(gralloc->alloc_buffer(the_properties));
    }
    front_buffer = free_queue.front();
}

void mc::BufferQueue::client_acquire(std::function<void(graphics::Buffer* buffer)> complete)
{
    std::unique_lock<decltype(guard)> lock(guard);
    give_to_client = std::move(complete);

    if (!free_queue.empty())
    {
        auto buffer = pop(free_queue);
        give_buffer_to_client(buffer, std::move(lock));
        return;
    }

    /* No empty buffers, attempt allocating more*/
    if (allocated_buffers < nbuffers)
    {
        auto buffer = gralloc->alloc_buffer(the_properties);
        allocated_buffers++;
        give_buffer_to_client(buffer, std::move(lock));
        return;
    }

    /* Last resort, drop oldest buffer from the ready queue */
    if (frame_dropping_enabled && !ready_to_composite_queue.empty())
    {
        auto buffer = pop(ready_to_composite_queue);
        give_buffer_to_client(buffer, std::move(lock));
    }

    /* TODO: if there are no free or ready buffers available and frame
     * dropping is enabled, should they be stolen from the compositor?
     */
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

    auto buffer = pop(buffers_owned_by_client);
    ready_to_composite_queue.push_back(buffer);
}

std::shared_ptr<mg::Buffer>
mc::BufferQueue::compositor_acquire(void const* user_id)
{
    std::lock_guard<decltype(guard)> lock(guard);

    bool use_current_front_buffer =
        ready_to_composite_queue.empty() ||
        is_new_front_buffer_user(user_id);

    if (!use_current_front_buffer)
    {
        /* The front buffer is being updated
         * so there are no users for it yet*/
        front_buffer_users.clear();

        front_buffer = pop(ready_to_composite_queue);

        /* Only the buffers taken from the ready queue are "owned"
         * by the compositor(s). If no buffers are ready, then the
         * last client updated buffer will be used instead, which
         * may be already owned by the client or owned by the free
         * queue.
         */
        buffers_owned_by_compositor.push_back(front_buffer);
    }

    /* A distinction is made between buffers sent to compositor(s)
     * and those truly owned by the compositors. buffers_sent_to_compositor
     * is mainly used to track reference counts and to check
     * the buffer parameter at release.
     */
    buffers_sent_to_compositor.push_back(front_buffer);
    front_buffer_users.push_back(user_id);
    return front_buffer;
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

    /* If we can't remove it it means compositor didn't own it.
     * This may happen when there weren't any ready buffers
     * when compositor acquire was called.
     */
    if (!remove(buffer, buffers_owned_by_compositor))
        return;

    if (give_to_client)
        give_buffer_to_client(buffer, std::move(lock));
    else
        free_queue.push_back(buffer);
}

std::shared_ptr<mg::Buffer> mc::BufferQueue::snapshot_acquire()
{
    std::unique_lock<decltype(guard)> lock(guard);
    pending_snapshots.push_back(front_buffer);
    return front_buffer;
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
        auto buffer = pop(ready_to_composite_queue);
        while (!ready_to_composite_queue.empty())
        {
            free_queue.push_back(pop(ready_to_composite_queue));
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

    /* Don't give to the client just yet, there's a pending
     * snapshot - avoid tearing artifacts.
     */
    if (!new_buffer && contains(buffer, pending_snapshots))
    {
        snapshot_released.wait(lock,
            [&]{ return !contains(buffer, pending_snapshots); });
    }

    buffers_owned_by_client.push_back(buffer);

    lock.unlock();
    give_to_client_cb(buffer.get());
}

bool mc::BufferQueue::is_new_front_buffer_user(void const* user_id)
{
    /* std::find could be used, but allocating iterators would
     * be more expensive than the search itself
     */
    if (!front_buffer_users.empty())
    {
        for (unsigned int i = 0; i < front_buffer_users.size(); i++)
        {
            if (user_id == front_buffer_users[i])
                return false;
        }
        return true;
    }
    /* The user list is really only empty the very first time.
     * False is returned so that the front buffer can be updated.
     */
    return false;
}
