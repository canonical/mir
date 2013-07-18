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
 * Authored by:
 * Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/graphic_buffer_allocator.h"
#include "switching_bundle.h"
#include <cassert>

namespace mc=mir::compositor;

mc::SwitchingBundle::SwitchingBundle(
    std::shared_ptr<GraphicBufferAllocator> &gralloc, BufferProperties const& property_request)
    : bundle_properties{property_request},
      gralloc{gralloc},
      nbuffers{MAX_BUFFERS},
      client{-1},
      ready{-1},
      compositor{-1},
      ncompositors{0},
      framedropping{true}
{
    for (int i = 0; i < MAX_BUFFERS; i++)
        ring[i] = gralloc->alloc_buffer(bundle_properties);
}

std::shared_ptr<mc::Buffer> mc::SwitchingBundle::client_acquire()
{
    std::unique_lock<std::mutex> lock(guard);

    if (framedropping)
    {
        while (client >= 0)
            cond.wait(lock);
    
        client = (compositor + 1) % nbuffers;
        if (client == ready)
            client = (client + 1) % nbuffers;
    }
    else
    {
        while (client >= 0 || ready >= 0)
            cond.wait(lock);
    
        client = (compositor + 1) % nbuffers;
    }

    return ring[client];
}

void mc::SwitchingBundle::client_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);
    assert(ring[client] == released_buffer);
    ready = client;
    client = -1;
    cond.notify_all();
}

std::shared_ptr<mc::Buffer> mc::SwitchingBundle::compositor_acquire()
{
    std::unique_lock<std::mutex> lock(guard);

    while (ready < 0)
        cond.wait(lock);

    compositor = ready;
    ready = -1;
    cond.notify_all();

    return ring[compositor];
}

void mc::SwitchingBundle::compositor_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    std::unique_lock<std::mutex> lock(guard);
    assert(ring[compositor] == released_buffer);
    compositor = -1;
}

void mc::SwitchingBundle::force_requests_to_complete()
{
    std::unique_lock<std::mutex> lock(guard);
    ready = -1;
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
