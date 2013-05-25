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
    return swapper->client_acquire();
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
}

void mc::SwapperSwitcher::switch_swapper(std::shared_ptr<mc::BufferSwapper> const& new_swapper)
{
    WriteLock wr_lk(rw_lock);
    std::unique_lock<mc::WriteLock> lk(wr_lk);
    // std::vector<Buffers> dirty, clean
    // swapper->dump(dirty, clean)
    // swapper->assume_buffers(dirty, clean)
    // swapper->force_completion();
    swapper = new_swapper;
}
