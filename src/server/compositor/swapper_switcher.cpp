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

#include <vector>
#include <mutex>

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
    while(!(buffer = swapper->client_acquire()))
    {
        //request was unservicable at the time
        cv.wait(lk);
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
    ReadLock rw_lk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rw_lk);
    return swapper->compositor_acquire();
}

void mc::SwapperSwitcher::compositor_release(std::shared_ptr<mc::Buffer> const& released_buffer)
{
    ReadLock rw_lk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rw_lk);
    return swapper->compositor_release(released_buffer);
}

void mc::SwapperSwitcher::force_requests_to_complete()
{
    ReadLock rw_lk(rw_lock);
    std::unique_lock<mc::ReadLock> lk(rw_lk);
    swapper->force_requests_to_complete();
}

void mc::SwapperSwitcher::switch_swapper(std::shared_ptr<mc::BufferSwapper> const& /*new_swapper*/)
{
#if 0
    force_requests_to_complete();

    WriteLock wr_lk(rw_lock);
    std::unique_lock<mc::WriteLock> lk(wr_lk);
    std::vector<std::shared_ptr<mc::Buffer>> dirty, clean;

    swapper->transfer_buffers_to(new_swapper);
    swapper = new_swapper;

    cv.notify_all();
#endif
}
