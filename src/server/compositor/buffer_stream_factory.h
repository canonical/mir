/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_COMPOSITOR_BUFFER_STREAM_FACTORY_H_
#define MIR_COMPOSITOR_BUFFER_STREAM_FACTORY_H_

#include "mir/scene/buffer_stream_factory.h"

#include <memory>

namespace mir
{
namespace graphics
{
class GraphicBufferAllocator;
}
namespace compositor
{

class BufferStreamFactory : public scene::BufferStreamFactory
{
public:
    BufferStreamFactory();

    virtual ~BufferStreamFactory() {}

    virtual std::shared_ptr<compositor::BufferStream> create_buffer_stream(
        int nbuffers,
        graphics::BufferProperties const& buffer_properties) override;
    virtual std::shared_ptr<BufferStream> create_buffer_stream(
        graphics::BufferProperties const&) override;
};

}
}


#endif /* MIR_COMPOSITOR_BUFFER_STREAM_FACTORY_H_ */
