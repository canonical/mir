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
#include "buffer_texture_binder.h"
#include "mir/geometry/dimensions.h"

#include <cstdint>
#include <memory>
#include <atomic>

namespace mir
{
namespace compositor
{

class GraphicBufferAllocator;
class BufferManagerClient;
class BufferManager : public BufferTextureBinder
{
 public:

    explicit BufferManager(GraphicBufferAllocator* gr_allocator);
    virtual ~BufferManager() {}
 
    virtual BufferManagerClient* create_client(geometry::Width width,
                                   geometry::Height height,
                                   PixelFormat pf);

    virtual bool register_buffer(std::shared_ptr<Buffer> buffer);

    // From buffer_texture_binder
    virtual void bind_buffer_to_texture(surfaces::SurfacesToRender const& surface);

 private:
    GraphicBufferAllocator* const gr_allocator;

    std::atomic<int> client_counter;
};

}
}


#endif /* MIR_COMPOSITOR_BUFFER_MANAGER_H_ */
