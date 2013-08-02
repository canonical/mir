/*
 * Copyright © 2012 Canonical Ltd.
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
 *  Alan Griffiths <alan@octopull.co.uk>
 *  Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_
#define MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_

#include "mir/graphics/buffer.h"

#include <vector>
#include <memory>

namespace mir
{
namespace graphics { struct BufferProperties; }

namespace compositor
{
class BufferSwapper;

enum class SwapperType
{
    synchronous,
    framedropping,
    bypass
};

class BufferAllocationStrategy
{
public:
    virtual std::shared_ptr<BufferSwapper> create_swapper_reuse_buffers(graphics::BufferProperties const&,
        std::vector<std::shared_ptr<graphics::Buffer>>&, size_t, SwapperType) const = 0;
    virtual std::shared_ptr<BufferSwapper> create_swapper_new_buffers(
        graphics::BufferProperties& actual_properties, graphics::BufferProperties const& requested_properties, SwapperType) const = 0;

protected:
    BufferAllocationStrategy() {}
    virtual ~BufferAllocationStrategy() { /* TODO: make nothrow */ }

    BufferAllocationStrategy(const BufferAllocationStrategy&);
    BufferAllocationStrategy& operator=(const BufferAllocationStrategy& );
};

}
}

#endif // MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_
