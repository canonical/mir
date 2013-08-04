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

#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_swapper_exceptions.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "switching_bundle.h"

#include <boost/throw_exception.hpp>

namespace mc=mir::compositor;
namespace mg = mir::graphics;

mc::SwitchingBundle::SwitchingBundle(
    std::shared_ptr<BufferAllocationStrategy> const& swapper_factory, mg::BufferProperties const& property_request)
    : swapper_factory(swapper_factory),
      swapper(swapper_factory->create_swapper_new_buffers(
                  bundle_properties, property_request, mc::SwapperType::synchronous))
{
}

std::shared_ptr<mg::Buffer> mc::SwitchingBundle::client_acquire()
{
    /* lock is for use of 'swapper' below' */
    std::unique_lock<mc::ReadLock> lk(rw_lock);

    std::shared_ptr<mg::Buffer> buffer = nullptr;
    while (!buffer) 
    {
        try
        {
            buffer = swapper->client_acquire();
        }
        catch (BufferSwapperRequestAbortedException const& e)
        {
            /* wait for the swapper to change before retrying */
            cv.wait(lk);
        }
    }
    return buffer;
}

void mc::SwitchingBundle::client_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<mc::ReadLock> lk(rw_lock);
    return swapper->client_release(released_buffer);
}

std::shared_ptr<mg::Buffer> mc::SwitchingBundle::compositor_acquire()
{
    std::unique_lock<mc::ReadLock> lk(rw_lock);
    return swapper->compositor_acquire();
}

void mc::SwitchingBundle::compositor_release(std::shared_ptr<mg::Buffer> const& released_buffer)
{
    std::unique_lock<mc::ReadLock> lk(rw_lock);
    return swapper->compositor_release(released_buffer);
}

void mc::SwitchingBundle::force_requests_to_complete()
{
    std::unique_lock<mc::ReadLock> lk(rw_lock);
    swapper->force_requests_to_complete();
}

void mc::SwitchingBundle::allow_framedropping(bool allow_dropping)
{
    {
        std::unique_lock<mc::ReadLock> lk(rw_lock);
        swapper->force_client_abort();
    }

    std::unique_lock<mc::WriteLock> lk(rw_lock);
    std::vector<std::shared_ptr<mg::Buffer>> list{};
    size_t size = 0;
    swapper->end_responsibility(list, size);

    if (allow_dropping)
        swapper = swapper_factory->create_swapper_reuse_buffers(bundle_properties, list, size, mc::SwapperType::framedropping);
    else
        swapper = swapper_factory->create_swapper_reuse_buffers(bundle_properties, list, size, mc::SwapperType::synchronous);

    cv.notify_all();
}

mg::BufferProperties mc::SwitchingBundle::properties() const
{
    return bundle_properties;
}
