/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_COMPOSITOR_BUFFER_MANAGER_H_
#define MIR_COMPOSITOR_BUFFER_MANAGER_H_

#include "buffer.h"
#include "mir/geometry/dimensions.h"

#include <cstdint>
#include <memory>
#include <atomic>
#include <list>

namespace mir
{
namespace compositor
{

class GraphicBufferAllocator;
class BufferBundle;
class BufferManager
{
 public:

    explicit BufferManager(GraphicBufferAllocator* gr_allocator);
    virtual ~BufferManager() {}

    virtual std::shared_ptr<BufferBundle> create_client(geometry::Width width,
                                   geometry::Height height,
                                   PixelFormat pf);

    virtual bool is_empty();
    virtual void destroy_client(std::shared_ptr<BufferBundle> client);

 private:
    GraphicBufferAllocator* const gr_allocator;

    std::list<std::shared_ptr<BufferBundle>> client_list; 

};

}
}


#endif /* MIR_COMPOSITOR_BUFFER_MANAGER_H_ */
