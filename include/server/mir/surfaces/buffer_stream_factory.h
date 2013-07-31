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

#ifndef MIR_SURFACES_BUFFER_STREAM_FACTORY_H_
#define MIR_SURFACES_BUFFER_STREAM_FACTORY_H_

#include <memory>

namespace mir
{
namespace graphics { struct BufferProperties; }

namespace surfaces
{
class BufferStream;

class BufferStreamFactory
{
public:
    virtual ~BufferStreamFactory() {}

    virtual std::shared_ptr<BufferStream> create_buffer_stream(
        graphics::BufferProperties const& buffer_properties) = 0;

protected:
    BufferStreamFactory() = default;
    BufferStreamFactory(const BufferStreamFactory&) = delete;
    BufferStreamFactory& operator=(const BufferStreamFactory&) = delete;
};

}
}

#endif // MIR_SURFACES_BUFFER_STREAM_FACTORY_H_
