/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_BUFFER_IPC_PACKER_H_
#define MIR_GRAPHICS_BUFFER_IPC_PACKER_H_

#include "mir/geometry/dimensions.h"

namespace mir
{
namespace graphics
{

class BufferIPCPacker
{
public:
    virtual ~BufferIPCPacker() = default;
    virtual void pack_fd(int) = 0;
    virtual void pack_data(int) = 0;
    virtual void pack_stride(geometry::Stride) = 0;
    virtual void pack_flags(unsigned int) = 0;

protected:
    BufferIPCPacker() {}
    BufferIPCPacker(BufferIPCPacker const&) = delete;
    BufferIPCPacker& operator=(BufferIPCPacker const&) = delete;

};

}
}

#endif /* MIR_GRAPHICS_BUFFER_IPC_PACKER_H_ */
