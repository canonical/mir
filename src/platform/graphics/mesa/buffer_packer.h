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

#ifndef MIR_GRAPHICS_MESA_BUFFER_PACKER_H_
#define MIR_GRAPHICS_MESA_BUFFER_PACKER_H_

#include "mir/graphics/buffer_ipc_packer.h"

namespace mir
{
namespace graphics
{
namespace mesa
{
class BufferPacker : public BufferIpcPacker
{
public:
    void pack_buffer(BufferIpcMessage& message, Buffer const& buffer, BufferIpcMsgType msg_type) const override;
    void unpack_buffer(BufferIpcMessage& message, Buffer const& buffer) const override;
    std::shared_ptr<PlatformIPCPackage> get_ipc_package() override;
};
}
}
}
#endif /* MIR_GRAPHICS_MESA_BUFFER_PACKER_H_ */
