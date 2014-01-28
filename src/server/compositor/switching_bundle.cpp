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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 *
 *
 * The "bundle" of buffers is actually a ring (circular array) from which
 * buffers are allocated and progress through stages.
 *
 * The stages of a buffer are:
 *   free -> client -> ready -> compositor <-> free
 *                     ready (dropped)-> free
 *
 * Dropping only happens when it's enabled, and only if the ring is
 * completely full.
 *
 * The successive stages are contiguous elements in the ring (starting at
 * element "first_compositor"):
 *    first_compositor * ncompositors(zero or more)
 *    first_ready      * nready(zero or more)
 *    first_client     * nclients(zero or more)
 *
 * Therefore you will find:
 *    first_compositor + ncompositors == first_ready
 *    first_ready + nready == first_client
 * although the ring wraps around, so all addition is modulo (%) nbuffers.
 *
 * "free" is an implicit state for any buffer that is not in any of the
 * above three groups. So the next free buffer is always:
 *    first_client + nclients
 * and free buffers extend up to but not including first_compositor.
 *
 *  |<--------------------- nbuffers ----------------------->|
 *             | ncompos |    nready    | nclients|
 *  +----------+---------+--------------+---------+----------+
 *  | ... free |    |    |    |    |    |    |    | free ... |
 *  +----------+---------+--------------+---------+----------+
 *               ^         ^ first_ready  ^ first_client
 *               first_compositor
 *
 * Finally, there is a floating pointer called "snapshot" which can point to
 * any buffer other than an active client. If nsnappshotters>0 then "snapshot"
 * currently points to a buffer that is being snapshotted by one or more
 * threads.
 */

#include "mir/graphics/graphic_buffer_allocator.h"
#include "switching_bundle.h"

#include <boost/throw_exception.hpp>
#include <utility>
#include <ostream>

namespace mc=mir::compositor;
namespace mg = mir::graphics;

mc::SwitchingBundle::SwitchingBundle(
    int nbuffers,
    const std::shared_ptr<graphics::GraphicBufferAllocator> &gralloc,
    const mg::BufferProperties &property_request)
    : bundle_properties{property_request},
      gralloc{gralloc},
      nbuffers{nbuffers},
      first_compositor{0}, ncompositors{0},
      first_ready{0}, nready{0},
      first_client{0}, nclients{0},
      snapshot{-1}, nsnapshotters{0},
      overlapping_compositors{false},
      framedropping{false}, force_drop{0}
{
    if (nbuffers < min_buffers || nbuffers > max_buffers)
    {
        BOOST_THROW_EXCEPTION(std::logic_error("SwitchingBundle only supports "
                                               "nbuffers between " +
                                               std::to_string(min_buffers) +
                                               " and " +
                                               std::to_string(max_buffers)));
    }
}

int mc::SwitchingBundle::nfree() const
{
    return nbuffers - ncompositors - nready - nclients;
}

int mc::SwitchingBundle::first_free() const
{
    return (first_client + nclients) % nbuffers;
}

int mc::SwitchingBundle::next(int slot) const
{
    return (slot + 1) % nbuffers;
}

int mc::SwitchingBundle::prev(int slot) const
{
    return (slot + nbuffers - 1) % nbuffers;
}

int mc::SwitchingBundle::last_compositor() const
{
    return (first_compositor + ncompositors - 1) % nbuffers;
}

int mc::SwitchingBundle::drop_frames(int max)
{
    // Drop up to max of the oldest ready frames, and put them on the free list
    int dropped = (max > nready) ? nready : max;

    for (int d = 0; d < dropped; d++)
    {
        auto tmp = ring[first_ready];
        nready--;
        first_client = prev(first_client);
        int end = first_free();
        int new_snapshot = snapshot;

        for (int i = first_ready, j = 0; i != end; i = j)
        {
            j = next(i);
            ring[i] = ring[j];
            if (j == snapshot)
                new_snapshot = i;
        }

        if (snapshot == first_ready)
            new_snapshot = end;

        snapshot = new_snapshot;

        ring[end] = tmp;
    }

    return dropped;
}

const std::shared_ptr<mg::Buffer> &mc::SwitchingBundle::alloc_buffer(int slot)
{
    /*
     * Many clients will behave in a way that never requires more than 2 or 3
     * buffers, even though nbuffers may be higher. So to optimize memory
     * usage for this common case, try to avoid allocating new buffers if
     * we don't need to.
     */
    if (!ring[slot].buf)
    {
        int i = first_free();
        while (i != first_compositor && !ring[i].buf)
            i = next(i);

        if (i != first_compositor &&
            i != slot &&
            ring[i].buf &&
            ring[i].buf.unique())
        {
            std::swap(ring[slot], ring[i]);
        }
        else
        {
            ring[slot].buf = gralloc->alloc_buffer(bundle_properties);
        }
    }

    return ring[slot].buf;
}

mg::Buffer* mc::SwitchingBundle::client_acquire()
{
    std::unique_lock<std::mutex> lock(guard);

    if ((framedropping || force_drop) && nbuffers > 1)
    {
        if (nfree() <= 0)
        {
            while (nready == 0)
                cond.wait(lock);

            drop_frames(1);
        }
    }
    else
    {
        /*
         * Even if there are free buffers available, we might wish to still
         * wait. This is so we don't touch (hence allocate) a third or higher
         * buffer until/unless it's absolutely necessary. It becomes necessary
         * only when framedropping (above) or with overlapping compositors
         * (like with bypass).
         * The performance benefit of triple buffering is usually minimal,
         * but always uses 50% more memory. So try to avoid it when possible.
         */

        int min_free =
#if 0  // FIXME: This memory optimization breaks timing tests
            (nbuffers > 2 && !overlapping_compositors) ?  nbuffers - 1 : 1;
#else
            1;
#endif

        while (nfree() < min_free)
            cond.wait(lock);
    }

    if (force_drop > 0)
        force_drop--;

    int client = first_free();
    nclients++;

    auto ret = alloc_buffer(client);
    if (client == snapshot)
    {
        /*
         * In the rare case where a snapshot is still being taken of what is
         * the new client buffer, try to move it out the way, or if you can't
         * then wait for the snapshot to finish.
         */
        if (nfree() > 0)
        {
            snapshot = next(client);
            std::swap(ring[snapshot], ring[client]);
            ret = alloc_buffer(client);
        }
        else
        {
            while (client == snapshot)
                cond.wait(lock);
        }
    }

    if (ret->size() != bundle_properties.size)
    {
        ret = gralloc->alloc_buffer(bundle_properties);
        ring[client].buf = ret;
    }

    return ret.get();
}

void mc::SwitchingBundle::client_release(graphics::Buffer* released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);

    if (nclients <= 0 || ring[first_client].buf.get() != released_buffer)
    {
        BOOST_THROW_EXCEPTION(std::logic_error(
            "Client release out of order"));
    }

    first_client = next(first_client);
    nclients--;
    nready++;
    cond.notify_all();
}

std::shared_ptr<mg::Buffer> mc::SwitchingBundle::compositor_acquire(
    unsigned long frameno)
{
    std::unique_lock<std::mutex> lock(guard);
    int compositor;

    // Multi-monitor acquires close to each other get the same frame:
    bool same_frame = last_consumed && (frameno == *last_consumed);

    int avail = nfree();
    bool can_recycle = ncompositors || avail;

    if (!nready || (same_frame && can_recycle))
    {
        if (ncompositors)
        {
            compositor = last_compositor();
        }
        else if (avail)
        {
            first_compositor = prev(first_compositor);
            compositor = first_compositor;
            ncompositors++;
        }
        else
        {
            BOOST_THROW_EXCEPTION(std::logic_error(
                "compositor_acquire would block; probably too many clients."));
        }
    }
    else
    {
        compositor = first_ready;
        first_ready = next(first_ready);
        nready--;
        ncompositors++;
        last_consumed = frameno;
    }

    overlapping_compositors = (ncompositors > 1);

    ring[compositor].users++;
    return alloc_buffer(compositor);
}

void mc::SwitchingBundle::compositor_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);
    int compositor = -1;

    for (int n = 0, i = first_compositor; n < ncompositors; n++, i = next(i))
    {
        if (ring[i].buf == released_buffer)
        {
            compositor = i;
            break;
        }
    }

    if (compositor < 0)
    {
        BOOST_THROW_EXCEPTION(std::logic_error(
            "compositor_release given a non-compositor buffer"));
    }

    ring[compositor].users--;
    if (compositor == first_compositor)
    {
        while (!ring[first_compositor].users && ncompositors)
        {
            first_compositor = next(first_compositor);
            ncompositors--;
        }
        cond.notify_all();
    }
}

std::shared_ptr<mg::Buffer> mc::SwitchingBundle::snapshot_acquire()
{
    std::unique_lock<std::mutex> lock(guard);

    /*
     * Note that "nsnapshotters" is a separate counter to ring[x].users.
     * This is because snapshotters should be completely passive and should
     * not affect the compositing logic.
     */

    if (!nsnapshotters)
    {
        /*
         * Always snapshot the newest complete frame, which is always the
         * one before the first client. But make sure there is at least one
         * non-client buffer first...
         */
        while (nclients >= nbuffers)
            cond.wait(lock);

        if (!nsnapshotters)
            snapshot = prev(first_client);
    }

    nsnapshotters++;
    return alloc_buffer(snapshot);
}

void mc::SwitchingBundle::snapshot_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);

    if (nsnapshotters <= 0 || ring[snapshot].buf != released_buffer)
    {
        BOOST_THROW_EXCEPTION(std::logic_error(
            "snapshot_release passed a non-snapshot buffer"));
    }

    nsnapshotters--;

    if (!nsnapshotters)
        snapshot = -1;

    cond.notify_all();
}

void mc::SwitchingBundle::force_requests_to_complete()
{
    std::unique_lock<std::mutex> lock(guard);
    drop_frames(nready);
    force_drop = nbuffers + 1;
    cond.notify_all();
}

void mc::SwitchingBundle::allow_framedropping(bool allow_dropping)
{
    std::unique_lock<std::mutex> lock(guard);
    framedropping = allow_dropping;
}

bool mc::SwitchingBundle::framedropping_allowed() const
{
    std::unique_lock<std::mutex> lock(guard);
    return framedropping;
}

mg::BufferProperties mc::SwitchingBundle::properties() const
{
    std::unique_lock<std::mutex> lock(guard);
    return bundle_properties;
}

void mc::SwitchingBundle::resize(const geometry::Size &newsize)
{
    std::unique_lock<std::mutex> lock(guard);
    bundle_properties.size = newsize;
}

std::ostream& mc::operator<<(std::ostream& os, const mc::SwitchingBundle& bundle)
{
    os << "("
        << (void*)(&bundle)
        << ",nbuffers=" << bundle.nbuffers
        << ",first_compositor=" << bundle.first_compositor
        << ",ncompositors=" << bundle.ncompositors
        << ",first_ready=" << bundle.first_ready
        << ",nready=" << bundle.nready
        << ",first_client=" << bundle.first_client
        << ",nclients=" << bundle.nclients
        << ")";

    return os;
}
