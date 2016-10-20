/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_IPC_OPERATIONS_H_
#define MIR_GRAPHICS_NESTED_IPC_OPERATIONS_H_

#include "mir/graphics/platform_ipc_operations.h"
#include <memory>

namespace mir
{
namespace graphics
{
namespace nested
{
class IpcOperations : public PlatformIpcOperations
{
public:
    IpcOperations(std::shared_ptr<graphics::PlatformIpcOperations> const& host_operations);

    void pack_buffer(BufferIpcMessage& message, graphics::Buffer const& buffer, BufferIpcMsgType msg_type) const override;
    void unpack_buffer(BufferIpcMessage& message, graphics::Buffer const& buffer) const override;
    std::shared_ptr<PlatformIPCPackage> connection_ipc_package() override;
    PlatformOperationMessage platform_operation(
        unsigned int const opcode, PlatformOperationMessage const& message) override;

private:
    std::shared_ptr<graphics::PlatformIpcOperations> const ipc_operations;
};
}
}
}
#endif /* MIR_GRAPHICS_NESTED_IPC_OPERATIONS_H_ */
