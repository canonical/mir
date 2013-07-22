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
 *   free -> client -> ready -> compositor -> free
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
 */

#include "mir/compositor/graphic_buffer_allocator.h"
#include "switching_bundle.h"

#include <boost/throw_exception.hpp>

namespace mc=mir::compositor;
namespace mg = mir::graphics;

mc::SwitchingBundle::SwitchingBundle(
    const std::shared_ptr<GraphicBufferAllocator> &gralloc,
    const BufferProperties &property_request)
    : bundle_properties{property_request},
      gralloc{gralloc},
      nbuffers{2},
      first_compositor{0}, ncompositors{0},
      first_ready{0}, nready{0},
      first_client{0}, nclients{0},
      framedropping{false}
{
    if (nbuffers < 1)
        BOOST_THROW_EXCEPTION(std::logic_error("SwitchingBundle requires a "
                                               "positive number of buffers"));

    ring = new std::shared_ptr<graphics::Buffer>[nbuffers];
    for (int i = 0; i < nbuffers; i++)
        ring[i] = gralloc->alloc_buffer(bundle_properties);
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
    int drop = max;

    if (drop > nready)
        drop = nready;

    for (int d = 0; d < drop; d++)
    {
        first_client = (first_client + nbuffers - 1) % nbuffers;
        nready--;
        auto tmp = ring[first_client];
        int end = (first_client + nclients) % nbuffers;

        for (int i = first_client; i != end; i = (i + 1) % nbuffers)
            ring[i] = ring[(i + 1) % nbuffers];

        ring[end] = tmp;
    }

    return drop;
}

std::shared_ptr<mg::Buffer> mc::SwitchingBundle::client_acquire()
{
    std::unique_lock<std::mutex> lock(guard);

    if (framedropping && nbuffers > 1)
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
        while (nfree() <= 0)
            cond.wait(lock);
    }

    int client = (first_client + nclients) % nbuffers;
    nclients++;

    return ring[client];
}

void mc::SwitchingBundle::client_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);

    if (ring[first_client] != released_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error(
            "Client release out of order"));
        
    first_client = (first_client + 1) % nbuffers;
    nclients--;
    nready++;
    cond.notify_all();
}

std::shared_ptr<mg::Buffer> mc::SwitchingBundle::compositor_acquire()
{
    std::unique_lock<std::mutex> lock(guard);
    int compositor;

    if (nbuffers > 2 && !nready && nfree())
    {
        /*
         * If there's nothing else available then show an old frame.
         * This only works with 3 or more buffers. If you've only got 2 and
         * are frame dropping then the compositor would be starved of new
         * frames, forced to race for them.
         */
        first_compositor = (first_compositor + nbuffers - 1) % nbuffers;
        compositor = first_compositor;
    }
    else
    {
        while (nready <= 0)
            cond.wait(lock);
    
        compositor = first_ready;
        first_ready = (first_ready + 1) % nbuffers;
        nready--;
    }
    ncompositors++;

    return ring[compositor];
}

void mc::SwitchingBundle::compositor_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);

    if (ring[first_compositor] != released_buffer)
        BOOST_THROW_EXCEPTION(std::logic_error(
            "Compositor release out of order"));

    first_compositor = (first_compositor + 1) % nbuffers;
    ncompositors--;
    cond.notify_all();
}

void mc::SwitchingBundle::force_requests_to_complete()
{
    std::unique_lock<std::mutex> lock(guard);
    drop_frames(nready);
    cond.notify_all();
}

void mc::SwitchingBundle::allow_framedropping(bool allow_dropping)
{
    framedropping = allow_dropping;
}

mc::BufferProperties mc::SwitchingBundle::properties() const
{
    return bundle_properties;
}
