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
#include <cassert>

namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{
mg::Buffer* pop(std::deque<mg::Buffer*>& q)
{
    auto const buffer = q.front();
    q.pop_front();
    return buffer;
}

bool remove(mg::Buffer const* item, std::vector<mg::Buffer*>& list)
{
    int const size = list.size();
    for (int i = 0; i < size; ++i)
    {
        if (list[i] == item)
        {
            list.erase(list.begin() + i);
            return true;
        }
    }
    /* nothing removed*/
    return false;
}

bool contains(mg::Buffer const* item, std::vector<mg::Buffer*> const& list)
{
    int const size = list.size();
    for (int i = 0; i < size; ++i)
    {
        if (list[i] == item)
            return true;
    }
    return false;
}

std::shared_ptr<mg::Buffer> const&
buffer_for(mg::Buffer const* item, std::vector<std::shared_ptr<mg::Buffer>> const& list)
{
    int const size = list.size();
    for (int i = 0; i < size; ++i)
    {
        if (list[i].get() == item)
            return list[i];
    }
    BOOST_THROW_EXCEPTION(std::logic_error("buffer for pointer not found in list"));
}

void replace(mg::Buffer const* item, std::shared_ptr<mg::Buffer> const& new_buffer,
    std::vector<std::shared_ptr<mg::Buffer>>& list)
{
    int size = list.size();
    for (int i = 0; i < size; ++i)
    {
        if (list[i].get() == item)
        {
            list[i] = new_buffer;
            return;
        }
    }
    BOOST_THROW_EXCEPTION(std::logic_error("item to replace not found in list"));
}
}

mc::BufferQueue::BufferQueue(
    int nbuffers,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& gralloc,
    graphics::BufferProperties const& props,
    mc::FrameDroppingPolicyFactory const& policy_provider)
    : nbuffers{nbuffers},
      frame_dropping_enabled{false},
      the_properties{props},
      force_new_compositor_buffer{false},
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
    for(int i = 0; i < std::min(nbuffers, 2); i++)
    {
        buffers.push_back(gralloc->alloc_buffer(the_properties));
    }

    /* N - 1 for clients, one for compositors */
    int const buffers_for_client = buffers.size() - 1;
    for(int i = 0; i < buffers_for_client; i++)
    {
        free_buffers.push_back(buffers[i].get());
    }

    current_compositor_buffer = buffers.back().get();
    /* Special case: with one buffer both clients and compositors
     * need to share the same buffer
     */
    if (nbuffers == 1)
        free_buffers.push_back(current_compositor_buffer);

    framedrop_policy = policy_provider.create_policy([this]
    {
       std::unique_lock<decltype(guard)> lock{guard};

       if (pending_client_notifications.empty())
       {
           /*
            * This framedrop handler may be in the process of being dispatched
            * when we try to cancel it by calling swap_unblocked() when we
            * get a buffer to give back to the client. In this case we cannot
            * cancel and this function may be called without any pending client
            * notifications. This is a benign race that we can deal with by
            * just ignoring the framedrop request.
            */
           return;
       }

       if (ready_to_composite_queue.empty())
       {
           /*
            * NOTE: This can only happen under two circumstances:
            * 1) Client is single buffered. Don't do that, it's a bad idea.
            * 2) Client already has a buffer, and is asking for a new one
            *    without submitting the old one.
            *
            *    This shouldn't be exposed to the client as swap_buffers
            *    blocking, as the client already has something to render to.
            *
            *    Our current implementation will never hit this case. If we
            *    later want clients to be able to own multiple buffers simultaneously
            *    then we might want to add an entry to the CompositorReport here.
            */
           return;
       }
       drop_frame(std::move(lock));
    });
}

void mc::BufferQueue::client_acquire(mc::BufferQueue::Callback complete)
{
    std::unique_lock<decltype(guard)> lock(guard);

    pending_client_notifications.push_back(std::move(complete));

    if (!free_buffers.empty())
    {
        auto const buffer = free_buffers.back();
        free_buffers.pop_back();
        give_buffer_to_client(buffer, std::move(lock));
        return;
    }

    /* No empty buffers, attempt allocating more
     * TODO: need a good heuristic to switch
     * between double-buffering to n-buffering
     */
    int const allocated_buffers = buffers.size();
    if (allocated_buffers < nbuffers)
    {
        auto const& buffer = gralloc->alloc_buffer(the_properties);
        buffers.push_back(buffer);
        give_buffer_to_client(buffer.get(), std::move(lock));
        return;
    }

    /* Last resort, drop oldest buffer from the ready queue */
    if (frame_dropping_enabled && !ready_to_composite_queue.empty())
    {
        drop_frame(std::move(lock));
        return;
    }

    /* Can't give the client a buffer yet; they'll just have to wait
     * until the compositor is done with an old frame, or the policy
     * says they've waited long enough.
     */
    framedrop_policy->swap_now_blocking();
}

void mc::BufferQueue::client_release(graphics::Buffer* released_buffer)
{
    std::lock_guard<decltype(guard)> lock(guard);

    if (buffers_owned_by_client.empty())
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("unexpected release: no buffers were given to client"));
    }

    if (buffers_owned_by_client.front() != released_buffer)
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("client released out of sequence"));
    }

    auto const buffer = pop(buffers_owned_by_client);
    ready_to_composite_queue.push_back(buffer);
}

std::shared_ptr<mg::Buffer>
mc::BufferQueue::compositor_acquire(void const* user_id)
{
    std::unique_lock<decltype(guard)> lock(guard);

    bool use_current_buffer = false;
    if (!current_buffer_users.empty() && !is_a_current_buffer_user(user_id))
    {
        use_current_buffer = true;
        current_buffer_users.push_back(user_id);
    }

    if (ready_to_composite_queue.empty())
    {
        use_current_buffer = true;
    }
    else if (force_new_compositor_buffer)
    {
        use_current_buffer = false;
        force_new_compositor_buffer = false;
    }

    mg::Buffer* buffer_to_release = nullptr;
    if (!use_current_buffer)
    {
        /* No other compositors currently reference this
         * buffer so release it
         */
        if (!contains(current_compositor_buffer, buffers_sent_to_compositor))
            buffer_to_release = current_compositor_buffer;

        /* The current compositor buffer is
         * being changed, the new one has no users yet
         */
        current_buffer_users.clear();
        current_buffer_users.push_back(user_id);
        current_compositor_buffer = pop(ready_to_composite_queue);
    }
    else if (current_buffer_users.empty())
    {   // current_buffer_users and ready_to_composite_queue both empty
        current_buffer_users.push_back(user_id);
    }

    buffers_sent_to_compositor.push_back(current_compositor_buffer);

    std::shared_ptr<mg::Buffer> const acquired_buffer =
        buffer_for(current_compositor_buffer, buffers);

    if (buffer_to_release)
        release(buffer_to_release, std::move(lock));

    return acquired_buffer;
}

void mc::BufferQueue::compositor_release(std::shared_ptr<graphics::Buffer> const& buffer)
{
    std::unique_lock<decltype(guard)> lock(guard);

    if (!remove(buffer.get(), buffers_sent_to_compositor))
    {
        BOOST_THROW_EXCEPTION(
            std::logic_error("unexpected release: buffer was not given to compositor"));
    }

    /* Not ready to release it yet, other compositors still reference this buffer */
    if (contains(buffer.get(), buffers_sent_to_compositor))
        return;

    if (nbuffers <= 1)
        return;

    /*
     * We can't release the current_compositor_buffer because we need to keep
     * a compositor buffer always-available. But there might be a new
     * compositor buffer available to take its place immediately. Moving to
     * that one immediately will free up the old compositor buffer, allowing
     * us to call back the client with a buffer where otherwise we couldn't.
     */
    if (current_compositor_buffer == buffer.get() &&
        !ready_to_composite_queue.empty())
    {
        current_compositor_buffer = pop(ready_to_composite_queue);

        // Ensure current_compositor_buffer gets reused by the next
        // compositor_acquire:
        current_buffer_users.clear();
        void const* const impossible_user_id = this;
        current_buffer_users.push_back(impossible_user_id);
    }

    if (current_compositor_buffer != buffer.get())
        release(buffer.get(), std::move(lock));
}

std::shared_ptr<mg::Buffer> mc::BufferQueue::snapshot_acquire()
{
    std::unique_lock<decltype(guard)> lock(guard);
    pending_snapshots.push_back(current_compositor_buffer);
    return buffer_for(current_compositor_buffer, buffers);
}

void mc::BufferQueue::snapshot_release(std::shared_ptr<graphics::Buffer> const& buffer)
{
    std::unique_lock<std::mutex> lock(guard);
    if (!remove(buffer.get(), pending_snapshots))
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
    if (!pending_client_notifications.empty() &&
        !ready_to_composite_queue.empty())
    {
        auto const buffer = pop(ready_to_composite_queue);
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

int mc::BufferQueue::buffers_free_for_client() const
{
    std::lock_guard<decltype(guard)> lock(guard);
    int ret = 1;
    if (nbuffers > 1)
    {
        int nfree = free_buffers.size();
        int future_growth = nbuffers - buffers.size();
        ret = nfree + future_growth;
    }
    return ret;
}

void mc::BufferQueue::give_buffer_to_client(
    mg::Buffer* buffer,
    std::unique_lock<std::mutex> lock)
{
    /* Clears callback */
    auto give_to_client_cb = std::move(pending_client_notifications.front());
    pending_client_notifications.pop_front();

    bool const resize_buffer = buffer->size() != the_properties.size;
    if (resize_buffer)
    {
        auto const& resized_buffer = gralloc->alloc_buffer(the_properties);
        replace(buffer, resized_buffer, buffers);
        buffer = resized_buffer.get();
        /* Special case: the current compositor buffer also needs to be
         * replaced as it's shared with the client
         */
        if (nbuffers == 1)
            current_compositor_buffer = buffer;
    }

    /* Don't give to the client just yet if there's a pending snapshot */
    if (!resize_buffer && contains(buffer, pending_snapshots))
    {
        snapshot_released.wait(lock,
            [&]{ return !contains(buffer, pending_snapshots); });
    }

    buffers_owned_by_client.push_back(buffer);

    lock.unlock();
    try
    {
        give_to_client_cb(buffer);
    }
    catch (...)
    {
        /* comms errors should not propagate to compositing threads */
    }
}

bool mc::BufferQueue::is_a_current_buffer_user(void const* user_id) const
{
    int const size = current_buffer_users.size();
    int i = 0;
    while (i < size && current_buffer_users[i] != user_id) ++i;
    return i < size;
}

void mc::BufferQueue::release(
    mg::Buffer* buffer,
    std::unique_lock<std::mutex> lock)
{
    if (!pending_client_notifications.empty())
    {
        framedrop_policy->swap_unblocked();
        give_buffer_to_client(buffer, std::move(lock));
    }
    else
        free_buffers.push_back(buffer);
}

void mc::BufferQueue::drop_frame(std::unique_lock<std::mutex> lock)
{
    auto buffer_to_give = pop(ready_to_composite_queue);
    /* Advance compositor buffer so it always points to the most recent
     * client content
     */
    if (!contains(current_compositor_buffer, buffers_sent_to_compositor))
    {
       current_buffer_users.clear();
       void const* const impossible_user_id = this;
       current_buffer_users.push_back(impossible_user_id);
       std::swap(buffer_to_give, current_compositor_buffer);
    }
    give_buffer_to_client(buffer_to_give, std::move(lock));
}

void mc::BufferQueue::drop_old_buffers()
{
    std::vector<mg::Buffer*> to_release;

    {
        std::lock_guard<decltype(guard)> lock{guard};
        force_new_compositor_buffer = true;

        while (ready_to_composite_queue.size() > 1)
            to_release.push_back(pop(ready_to_composite_queue));
    }

    for (auto buffer : to_release)
    {
       std::unique_lock<decltype(guard)> lock{guard};
       release(buffer, std::move(lock));
    }
}

void mc::BufferQueue::drop_client_requests()
{
    std::unique_lock<std::mutex> lock(guard);
    pending_client_notifications.clear();
}
