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
      framedropping{false}, force_drop{0}
{
    if (nbuffers < 1)
        BOOST_THROW_EXCEPTION(std::logic_error("SwitchingBundle requires a "
                                               "positive number of buffers"));

    ring = new SharedBuffer[nbuffers];
    for (int i = 0; i < nbuffers; i++)
        ring[i].users = 0;
}

mc::SwitchingBundle::~SwitchingBundle()
{
    delete[] ring;
}

int mc::SwitchingBundle::nfree() const
{
    return nbuffers - ncompositors - nready - nclients;
}

int mc::SwitchingBundle::drop_frames(int max)
{
    // Drop up to max of the oldest ready frames, and put them on the free list
    int dropped = (max > nready) ? nready : max;

    for (int d = 0; d < dropped; d++)
    {
        auto tmp = ring[first_ready];
        nready--;
        first_client = (first_ready + nready) % nbuffers;
        int end = (first_client + nclients) % nbuffers;
        int new_snapshot = snapshot;

        for (int i = first_ready, j = 0; i != end; i = j)
        {
            j = (i + 1) % nbuffers;
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
        int i = (first_client + nclients) % nbuffers;  // first free slot

        while (i != first_compositor && !ring[i].buf)
            i = (i + 1) % nbuffers;

        if (i != slot && ring[i].buf && ring[i].buf.unique())
        {
            auto tmp = ring[slot];
            ring[slot] = ring[i];
            ring[i] = tmp;
        }
        else
        {
            ring[slot].buf = gralloc->alloc_buffer(bundle_properties);
        }
    }

    return ring[slot].buf;
}

std::shared_ptr<mg::Buffer> mc::SwitchingBundle::client_acquire()
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
        while (!nfree())
            cond.wait(lock);
    }

    if (force_drop > 0)
        force_drop--;

    int client = (first_client + nclients) % nbuffers;
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
            snapshot = (client + 1) % nbuffers;
            auto tmp = ring[snapshot];
            ring[snapshot] = ring[client];
            ring[client] = tmp;
            ret = alloc_buffer(client);
        }
        else
        {
            while (client == snapshot)
                cond.wait(lock);
        }
    }

    return ret;
}

void mc::SwitchingBundle::client_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);

    if (nclients <= 0 || ring[first_client].buf != released_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error(
            "Client release out of order"));

    /*
     * If in synchronous mode, throttle the client to the refresh rate.
     * It's a little weird to do this in the release function, but doing it
     * here allows for maximum concurrency, so we always have a new client
     * buffer ready to give the compositor, even for slow clients.
     * See also the fix for LP: #1199450 in compositor_acquire.
     */
    while (!(framedropping || force_drop) && nready > 0)
        cond.wait(lock);

    first_client = (first_client + 1) % nbuffers;
    nclients--;
    nready++;
    cond.notify_all();
}

std::shared_ptr<mg::Buffer> mc::SwitchingBundle::compositor_acquire()
{
    std::unique_lock<std::mutex> lock(guard);
    int compositor;

    if (!nready)
    {
        if (ncompositors)
        {
            compositor = (first_compositor + ncompositors - 1) % nbuffers;
        }
        else if (nfree())
        {
            first_compositor = (first_compositor + nbuffers - 1) % nbuffers;
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
        /*
         * Make sure the compositor gets the latest frame (LP: #1199450)
         * This is arguably more of a workaround than a fix. The real issue
         * is that Mir's compositor doesn't have the smarts to understand
         * that a surface might have multiple frames ready, and therefore it
         * should schedule multiple page flips. If that is ever resolved, then
         * this drop_frames and the wait in client_release can be safely
         * removed.
         */
        drop_frames(nready - 1);

        compositor = first_ready;
        first_ready = (first_ready + 1) % nbuffers;
        nready--;
        ncompositors++;
    }

    ring[compositor].users++;
    return alloc_buffer(compositor);
}

void mc::SwitchingBundle::compositor_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);

    if (ncompositors <= 0 || ring[first_compositor].buf != released_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error(
            "Compositor release out of order"));

    if ((--ring[first_compositor].users) == 0)
    {
        first_compositor = (first_compositor + 1) % nbuffers;
        ncompositors--;
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
            snapshot = (first_client + nbuffers - 1) % nbuffers;
    }

    nsnapshotters++;
    return alloc_buffer(snapshot);
}

void mc::SwitchingBundle::snapshot_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);

    if (nsnapshotters <= 0 || ring[snapshot].buf != released_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error(
            "snapshot_release passed a non-snapshot buffer"));

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
    framedropping = allow_dropping;
}

bool mc::SwitchingBundle::framedropping_allowed() const
{
    return framedropping;
}

mg::BufferProperties mc::SwitchingBundle::properties() const
{
    return bundle_properties;
}
