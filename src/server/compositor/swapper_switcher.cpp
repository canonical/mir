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
    : swapper(initial_swapper),
      should_retry(false)
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

void mc::SwapperSwitcher::force_client_completion()
{
    ReadLock rlk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rlk);
    should_retry = false;
    swapper->force_client_completion();
}

void mc::SwapperSwitcher::end_responsibility(std::vector<std::shared_ptr<Buffer>>&, size_t&)
{
    //TODO
}

void mc::SwapperSwitcher::change_swapper(std::function<std::shared_ptr<BufferSwapper>
                                     (std::vector<std::shared_ptr<Buffer>>&, size_t&)> create_swapper)
{
    {
        ReadLock rlk(rw_lock);
        std::unique_lock<mc::ReadLock> lk(rlk);
        should_retry = true;
        swapper->force_client_completion();
    }

    WriteLock wlk(rw_lock);
    std::unique_lock<mc::WriteLock> lk(wlk);

    std::vector<std::shared_ptr<mc::Buffer>> list{};
    size_t size = 0;
    swapper->end_responsibility(list, size);
    swapper = create_swapper(list, size);
    cv.notify_all();
}
