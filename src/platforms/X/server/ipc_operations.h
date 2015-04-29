/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 *
 */

#ifndef MIR_GRAPHICS_X_IPC_OPERATIONS_H_
#define MIR_GRAPHICS_X_IPC_OPERATIONS_H_

#include "mir/graphics/platform_ipc_operations.h"
#include "display_helpers.h"

namespace mir
{
namespace graphics
{
namespace X
{

class IpcOperations : public PlatformIpcOperations
{
public:
    IpcOperations(std::shared_ptr<helpers::DRMHelper> const& drm);

    void pack_buffer(BufferIpcMessage& message, Buffer const& buffer, BufferIpcMsgType msg_type) const override;
    void unpack_buffer(BufferIpcMessage& message, Buffer const& buffer) const override;
    std::shared_ptr<PlatformIPCPackage> connection_ipc_package() override;
    PlatformOperationMessage platform_operation(
        unsigned int const opcode,
        PlatformOperationMessage const& message) override;

    std::shared_ptr<helpers::DRMHelper> const& drm;
};

}
}
}
#endif /* MIR_GRAPHICS_X_IPC_OPERATIONS_H_ */
