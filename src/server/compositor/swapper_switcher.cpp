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
#include "swapper_switcher.h"

#include <boost/throw_exception.hpp>

namespace mc=mir::compositor;

mc::SwapperSwitcher::SwapperSwitcher(std::shared_ptr<mc::BufferSwapper> const& initial_swapper)
    : swapper(initial_swapper)
{
}

std::shared_ptr<mc::Buffer> mc::SwapperSwitcher::client_acquire()
{
    ReadLock rw_lk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rw_lk);
    std::shared_ptr<mc::Buffer> buffer;

    mc::BufferSwapper* last_swapper = nullptr;
    while(swapper.get() != last_swapper)
    {
        try
        {
            last_swapper = swapper.get();
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
    ReadLock rw_lk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rw_lk);
    return swapper->client_release(released_buffer);
}

std::shared_ptr<mc::Buffer> mc::SwapperSwitcher::compositor_acquire()
{
    ReadLock rlk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rlk);
    return swapper->compositor_acquire();
}

void mc::SwapperSwitcher::compositor_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    ReadLock rlk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rlk);
    return swapper->compositor_release(released_buffer);
}

void mc::SwapperSwitcher::force_requests_to_complete()
{
    ReadLock rlk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rlk);
    should_retry = false;
    swapper->force_requests_to_complete();
}

void mc::SwapperSwitcher::transfer_buffers_to(std::shared_ptr<mc::BufferSwapper> const& new_swapper)
{
    {
        ReadLock rlk(rw_lock);
        std::unique_lock<mc::ReadLock> lk(rlk);
        should_retry = true;
        swapper->force_requests_to_complete();
    }

    WriteLock wlk(rw_lock);
    std::unique_lock<mc::WriteLock> lk(wlk);
    swapper->transfer_buffers_to(new_swapper);
    swapper = new_swapper;

    cv.notify_all();
}

void mc::SwapperSwitcher::adopt_buffer(std::shared_ptr<compositor::Buffer> const&)
{
}
