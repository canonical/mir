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

namespace mc=mir::compositor;

mc::SwitchingBundle::SwitchingBundle(
    std::shared_ptr<GraphicBufferAllocator> &gralloc, BufferProperties const& property_request)
    : bundle_properties{property_request},
      gralloc{gralloc}
{
    for (int i = 0; i < 3; i++)
        ring[i] = gralloc->alloc_buffer(bundle_properties);

    client = -1;
    ready = -1;
    compositor = -1;
}

std::shared_ptr<mc::Buffer> mc::SwitchingBundle::client_acquire()
{
    std::unique_lock<std::mutex> lock(guard);

    while (client >= 0)
        cond.wait(lock);

    client = (compositor + 1) % 3;
    if (client == ready)
        client = (client + 1) % 3;

    return ring[client];
}

void mc::SwitchingBundle::client_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    // TODO
    (void)released_buffer;

    std::unique_lock<std::mutex> lock(guard);
    ready = client;
    client = -1;
    cond.notify_one();
}

std::shared_ptr<mc::Buffer> mc::SwitchingBundle::compositor_acquire()
{
    std::unique_lock<std::mutex> lock(guard);

    while (ready < 0)
        cond.wait(lock);

    compositor = ready;
    ready = -1;

    return ring[compositor];
}

void mc::SwitchingBundle::compositor_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    // TODO
    (void)released_buffer;

    std::unique_lock<std::mutex> lock(guard);
    compositor = -1;
}

void mc::SwitchingBundle::force_requests_to_complete()
{
    // TODO, if anything?
}

void mc::SwitchingBundle::allow_framedropping(bool allow_dropping)
{
    // TODO
    (void)allow_dropping;
}

mc::BufferProperties mc::SwitchingBundle::properties() const
{
    return bundle_properties;
}
