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
#include "mir/compositor/buffer_allocation_strategy.h"
#include "swapper_switcher.h"

#include <boost/throw_exception.hpp>

namespace mc=mir::compositor;

mc::SwapperSwitcher::SwapperSwitcher(
    std::shared_ptr<BufferSwapper> const& initial_swapper,
    std::shared_ptr<BufferAllocationStrategy> const& swapper_factory)
    : swapper(initial_swapper),
      swapper_factory(swapper_factory),
      should_retry(false)
{
}

std::shared_ptr<mc::Buffer> mc::SwapperSwitcher::client_acquire()
{
    /* lock is for use of 'swapper' below' */
    std::unique_lock<mc::ReadLock> lk(rw_lock);

    std::shared_ptr<mc::Buffer> buffer = nullptr;
    while (!buffer) 
    {
        try
        {
            buffer = swapper->client_acquire();
        } catch (std::logic_error& e)
        {
            /* if failure is non recoverable, rethrow. */
            if (!should_retry || (e.what() != std::string("forced_completion")))
                throw e;

            /* wait for the swapper to change before retrying */
            cv.wait(lk);
        }
    }
    return buffer;
}

void mc::SwapperSwitcher::client_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    std::unique_lock<mc::ReadLock> lk(rw_lock);
    return swapper->client_release(released_buffer);
}

std::shared_ptr<mc::Buffer> mc::SwapperSwitcher::compositor_acquire()
{
    std::unique_lock<mc::ReadLock> lk(rw_lock);
    return swapper->compositor_acquire();
}

void mc::SwapperSwitcher::compositor_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    std::unique_lock<mc::ReadLock> lk(rw_lock);
    return swapper->compositor_release(released_buffer);
}

void mc::SwapperSwitcher::force_client_completion()
{
    std::unique_lock<mc::ReadLock> lk(rw_lock);
    should_retry = false;
    swapper->force_client_completion();
}

void mc::SwapperSwitcher::end_responsibility(std::vector<std::shared_ptr<Buffer>>&, size_t&)
{
    //TODO
}

#if 0
void mc::SwapperSwitcher::change_swapper(
    std::function<std::shared_ptr<BufferSwapper>(std::vector<std::shared_ptr<Buffer>>&, size_t&)> create_swapper)
{
    {
        std::unique_lock<mc::ReadLock> lk(rw_lock);
        should_retry = true;
        swapper->force_client_completion();
    }

    std::unique_lock<mc::WriteLock> lk(rw_lock);
    std::vector<std::shared_ptr<mc::Buffer>> list{};
    size_t size = 0;
    swapper->end_responsibility(list, size);
    swapper = create_swapper(list, size);
    cv.notify_all();
}
#endif

void mc::SwapperSwitcher::allow_framedropping(bool allow_dropping)
{
    {
        std::unique_lock<mc::ReadLock> lk(rw_lock);
        should_retry = true;
        swapper->force_client_completion();
    }

    std::unique_lock<mc::WriteLock> lk(rw_lock);
    std::vector<std::shared_ptr<mc::Buffer>> list{};
    size_t size = 0;
    swapper->end_responsibility(list, size);

    if (allow_dropping)
        swapper = swapper_factory->create_async_swapper_reuse(list, size); 
    else
        swapper = swapper_factory->create_sync_swapper_reuse(list, size); 

    cv.notify_all();
}
