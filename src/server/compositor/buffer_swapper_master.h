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

#ifndef MIR_COMPOSITOR_BUFFER_SWAPPER_MASTER_H_
#define MIR_COMPOSITOR_BUFFER_SWAPPER_MASTER_H_

#include "mir/compositor/buffer_swapper.h"

namespace mir
{
namespace compositor
{

//TODO: this interface is really doing the same thing as BufferBundle. This interface should be eliminated 
//      in favor of SwapperSwitcher using the BufferBundle interface
class BufferSwapperMaster : public BufferSwapper
{
public:
    virtual void change_swapper(
        std::function<std::shared_ptr<BufferSwapper>(std::vector<std::shared_ptr<Buffer>>&, size_t&)>) = 0;
    virtual ~BufferSwapperMaster() noexcept {}

protected:
    BufferSwapperMaster() = default;
    BufferSwapperMaster(BufferSwapperMaster const&) = delete;
    BufferSwapperMaster& operator=(BufferSwapperMaster const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_BUFFER_SWAPPER_H_ */
