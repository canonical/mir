/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *  Alan Griffiths <alan@octopull.co.uk>
 *  Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_
#define MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_

#include "mir/compositor/buffer.h"

#include <memory>

namespace mir
{
namespace compositor
{
class GraphicBufferAllocator;
class BufferSwapper;
class BufferProperties;

class BufferAllocationStrategy
{
public:

    virtual std::unique_ptr<BufferSwapper> create_swapper(
        BufferProperties& actual_buffer_properties,
        BufferProperties const& requested_buffer_properties) = 0;

protected:
    ~BufferAllocationStrategy() {}
    BufferAllocationStrategy() {}

    BufferAllocationStrategy(const BufferAllocationStrategy&);
    BufferAllocationStrategy& operator=(const BufferAllocationStrategy& );
};

}
}

#endif // MIR_COMPOSITOR_BUFFER_ALLOCATION_STRATEGY_H_
