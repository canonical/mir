/*
 * Copyright © 2014-2015 Canonical Ltd.
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
#include "mir/lockable_callback.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>

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

namespace mir
{
namespace compositor
{
class BufferQueue::LockableCallback : public mir::LockableCallback
{
public:
    LockableCallback(BufferQueue* const q)
        : q{q}
    {
    }

    void operator()() override
    {
        // We ignore any ongoing snapshotting as it could lead to deadlock.
       // In order to wait the guard_lock needs to be released; a BufferQueue::release
       // call can sneak in at that time from a different thread which
       // can invoke framedrop_policy methods
        q->drop_frame(guard_lock, BufferQueue::ignore_snapshot);
    }

    void lock() override
    {
        // This lock is aquired before the framedrop policy acquires any locks of its own.
        guard_lock = std::unique_lock<std::mutex>{q->guard};
    }

    void unlock() override
    {
        if (guard_lock.owns_lock())
            guard_lock.unlock();
    }

private:
    BufferQueue* const q;
    std::unique_lock<std::mutex> guard_lock;
};
}
}

mc::BufferQueue::BufferQueue(
    int nbuffers,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& gralloc,
    graphics::BufferProperties const& props,
    mc::FrameDroppingPolicyFactory const& policy_provider)
    : nbuffers{nbuffers},
      frame_dropping_enabled{false},
      current_compositor_buffer_valid{false},
      the_properties{props},
      force_new_compositor_buffer{false},
      callbacks_allowed{true},
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
    auto buf = gralloc->alloc_buffer(the_properties);
    buffers.push_back(buf);
    current_compositor_buffer = buf.get();

    /* Special case: with one buffer both clients and compositors
     * need to share the same buffer
     */
    if (nbuffers == 1)
        free_buffers.push_back(current_compositor_buffer);

    framedrop_policy = policy_provider.create_policy(
        std::make_shared<BufferQueue::LockableCallback>(this));
}

void mc::BufferQueue::client_acquire(mc::BufferQueue::Callback complete)
{
    std::unique_lock<decltype(guard)> lock(guard);

    pending_client_notifications.push_back(std::move(complete));

    if (!free_buffers.empty())
    {
        auto const buffer = free_buffers.back();
        free_buffers.pop_back();
        give_buffer_to_client(buffer, lock);
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
        give_buffer_to_client(buffer.get(), lock);
        return;
    }

    /* Last resort, drop oldest buffer from the ready queue */
    if (frame_dropping_enabled)
    {
        drop_frame(lock, wait_for_snapshot);
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
    if (!is_a_current_buffer_user(user_id))
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
        current_compositor_buffer_valid = true;
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
        give_buffer_to_client(buffer, lock, ignore_snapshot);
    }
}

void mc::BufferQueue::resize(geometry::Size const& new_size)
{
    std::lock_guard<decltype(guard)> lock(guard);
    the_properties.size = new_size;
}

int mc::BufferQueue::buffers_ready_for_compositor(void const* user_id) const
{
    std::lock_guard<decltype(guard)> lock(guard);

    int count = ready_to_composite_queue.size();
    if (!is_a_current_buffer_user(user_id))
    {
        // The virtual front of the ready queue isn't actually in the ready
        // queue, but is the current_compositor_buffer, so count that too:
        ++count;
    }

    return count;
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
    std::unique_lock<std::mutex>& lock)
{
    give_buffer_to_client(buffer, lock, wait_for_snapshot);
}

void mc::BufferQueue::give_buffer_to_client(
    mg::Buffer* buffer,
    std::unique_lock<std::mutex>& lock,
    SnapshotWait wait_type)
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
    if (wait_type == wait_for_snapshot && !resize_buffer && contains(buffer, pending_snapshots))
    {
        snapshot_released.wait(lock,
            [&]{ return !contains(buffer, pending_snapshots); });
    }

    if (!callbacks_allowed)  // We're shutting down
        return;

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
    if (!current_compositor_buffer_valid) return true;
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
        give_buffer_to_client(buffer, lock);
    }
    else if (!frame_dropping_enabled && buffers.size() > size_t(nbuffers))
    {
        /*
         * We're overallocated.
         * If frame_dropping_enabled, keep it that way to avoid having
         * to repeatedly reallocate. We must need the overallocation due to a
         * greedy compositor and insufficient nbuffers (LP: #1379685).
         * If not framedropping then we only overallocated to briefly
         * guarantee the framedropping policy and poke the client. Safe
         * to free it then because that's a rare occurrence.
         */
        for (auto i = buffers.begin(); i != buffers.end(); ++i)
        {
            if (i->get() == buffer)
            {
                buffers.erase(i);
                break;
            }
        }
    }
    else
        free_buffers.push_back(buffer);
}

void mc::BufferQueue::drop_frame(std::unique_lock<std::mutex>& lock, SnapshotWait wait_type)
{
    // Make sure there is a client waiting for the frame before we drop it.
    // If not, then there's nothing to do.
    if (pending_client_notifications.empty())
        return;

    mg::Buffer* buffer_to_give = nullptr;

    if (!free_buffers.empty())
    {  // We expect this to usually be empty, but always check free list first
        buffer_to_give = free_buffers.back();
        free_buffers.pop_back();
    }
    else if (!ready_to_composite_queue.empty() &&
             !contains(current_compositor_buffer, buffers_sent_to_compositor))
    {
        // Remember current_compositor_buffer is implicitly the front
        // of the ready queue.
        buffer_to_give = current_compositor_buffer;
        current_compositor_buffer = pop(ready_to_composite_queue);
        current_buffer_users.clear();
        current_compositor_buffer_valid = true;
    }
    else if (ready_to_composite_queue.size() > 1)
    {
        buffer_to_give = pop(ready_to_composite_queue);
    }
    else 
    {
        /*
         * Insufficient nbuffers for frame dropping? This means you're either
         * trying to use frame dropping with bypass/overlays/multimonitor or
         * have chosen nbuffers too low, or some combination thereof. So
         * consider the options...
         *  1. Crash. No, that's really unhelpful.
         *  2. Drop the visible frame. Probably not; that looks pretty awful.
         *     Not just tearing but you'll see very ugly polygon rendering
         *     artifacts.
         *  3. Drop the newest ready frame. Absolutely not; that will cause
         *     indefinite freezes or at least stuttering.
         *  4. Just give a warning and carry on at regular frame rate
         *     as if framedropping was disabled. That's pretty nice, but we
         *     can do better still...
         *  5. Overallocate; more buffers! Yes, see below.
         */
        auto const& buffer = gralloc->alloc_buffer(the_properties);
        buffers.push_back(buffer);
        buffer_to_give = buffer.get();
    }
        
    give_buffer_to_client(buffer_to_give, lock, wait_type);
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
    callbacks_allowed = false;
    pending_client_notifications.clear();
}
