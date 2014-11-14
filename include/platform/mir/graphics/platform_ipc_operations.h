/*
 * Copyright © 2014 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_PLATFORM_IPC_OPERATIONS_H_
#define MIR_GRAPHICS_PLATFORM_IPC_OPERATIONS_H_

#include "platform_ipc_package.h"
#include <memory>

namespace mir
{
namespace graphics
{
enum class BufferIpcMsgType
{
    full_msg, //pack the full ipc representation of the buffer
    update_msg //assume the client has a full representation, and pack only updates to the buffer 
};
class Buffer;
class BufferIpcMessage;

class PlatformIpcOperations
{
public:
    virtual ~PlatformIpcOperations() = default;
    /**
     * Arranges the IPC package for a buffer that is to be sent through
     * the frontend from server to client. This should be called every
     * time a buffer is to be sent cross-process.
     *
     * Pack the platform specific contents of Buffer into BufferIpcMessage for sending to the client
     *
     * \param [in] message   the message that will be sent
     * \param [in] buffer    the buffer to be put in the message
     * \param [in] ipc_type  what sort of ipc message is needed
     */
    virtual void pack_buffer(BufferIpcMessage& message, Buffer const& buffer, BufferIpcMsgType msg_type) const = 0;

    /**
     * Arranges the IPC package for a buffer that was sent over IPC
     * client to server. This must be called every time a buffer is
     * received, as some platform specific processing has to be done on
     * the incoming buffer.
     * \param [in] message   the message that was sent to the server
     * \param [in] buffer    the buffer associated with the message
     */
    virtual void unpack_buffer(BufferIpcMessage& message, Buffer const& buffer) const = 0;

    /**
     * Gets the connection package for the platform.
     *
     * The IPC package will be sent to clients when they connect.
     */
    virtual std::shared_ptr<PlatformIPCPackage> connection_ipc_package() = 0;

    
    /**
     * Arranges a platform specific operation triggered by an IPC call
     * \returns              the response that will be sent to the client
     * \param [in]  opcode   the opcode that indicates the action to be performed 
     * \param [in]  request  the message that was sent to the server
     */
    virtual PlatformIPCPackage platform_operation(unsigned int const opcode, PlatformIPCPackage const& package) = 0; 

protected:
    PlatformIpcOperations() = default;
    PlatformIpcOperations(PlatformIpcOperations const&) = delete;
    PlatformIpcOperations& operator=(PlatformIpcOperations const&) = delete;

};

}
}

#endif /* MIR_GRAPHICS_BUFFER_IPC_PACKER_H_ */
