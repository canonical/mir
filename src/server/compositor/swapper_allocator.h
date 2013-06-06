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

#ifndef MIR_COMPOSITOR_SWAPPER_ALLOCATOR_H_
#define MIR_COMPOSITOR_SWAPPER_ALLOCATOR_H_

#include <vector>
#include <memory>

namespace mir
{
namespace compositor
{
class BufferSwapper;
class Buffer;

class SwapperAllocator 
{
public:
    virtual ~SwapperAllocator() noexcept {}

    virtual std::shared_ptr<BufferSwapper> create_async_swapper(std::vector<std::shared_ptr<Buffer>>&, size_t) const = 0;
    virtual std::shared_ptr<BufferSwapper> create_sync_swapper(std::vector<std::shared_ptr<Buffer>>&, size_t) const = 0;
protected:
    SwapperAllocator() = default;
    SwapperAllocator(SwapperAllocator const&) = delete;
    SwapperAllocator& operator=(SwapperAllocator const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_SWAPPER_ALLOCATOR_H_ */
