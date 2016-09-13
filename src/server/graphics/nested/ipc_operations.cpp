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

#include "ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;

mgn::IpcOperations::IpcOperations(
    std::shared_ptr<mg::PlatformIpcOperations> const& host_operations) :
    ipc_operations(host_operations)
{
}

void mgn::IpcOperations::pack_buffer(
    mg::BufferIpcMessage& message, mg::Buffer const& buffer, mg::BufferIpcMsgType msg_type) const
{
    (void)message; (void)buffer; (void)msg_type;
}

void mgn::IpcOperations::unpack_buffer(mg::BufferIpcMessage& message, mg::Buffer const& buffer) const
{
    (void)message; (void)buffer;
}

std::shared_ptr<mg::PlatformIPCPackage> mgn::IpcOperations::connection_ipc_package()
{
    return nullptr;
}

mg::PlatformOperationMessage mgn::IpcOperations::platform_operation(
    unsigned int const opcode, mg::PlatformOperationMessage const& message)
{
    (void)opcode; (void) message;
    return mg::PlatformOperationMessage{};
}
