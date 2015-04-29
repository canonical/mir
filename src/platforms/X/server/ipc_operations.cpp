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

#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/platform_operation_message.h"
#include "ipc_operations.h"
#include "../debug.h"
#include "display_helpers.h"

#include <boost/throw_exception.hpp>
#include <iostream>

namespace mg = mir::graphics;
namespace mgx = mg::X;

mgx::IpcOperations::IpcOperations(std::shared_ptr<mgx::helpers::DRMHelper> const& drm)
    : drm{drm}
{
    CALLED
}

void mgx::IpcOperations::pack_buffer(
    mg::BufferIpcMessage& packer, Buffer const& buffer, BufferIpcMsgType msg_type) const
{
    CALLED

    if (msg_type == mg::BufferIpcMsgType::full_msg)
    {
        auto native_handle = buffer.native_buffer_handle();
        for(auto i=0; i<native_handle->data_items; i++)
        {
            packer.pack_data(native_handle->data[i]);
        }
        for(auto i=0; i<native_handle->fd_items; i++)
        {
            packer.pack_fd(mir::Fd(IntOwnedFd{native_handle->fd[i]}));
        }

        packer.pack_stride(buffer.stride());
        packer.pack_flags(native_handle->flags);
        packer.pack_size(buffer.size());
    }
}

void mgx::IpcOperations::unpack_buffer(BufferIpcMessage&, Buffer const&) const
{
    CALLED
}

mg::PlatformOperationMessage mgx::IpcOperations::platform_operation(
    unsigned int const /*op*/, mg::PlatformOperationMessage const& /*request*/)
{
    CALLED

    BOOST_THROW_EXCEPTION(std::invalid_argument("X platform does not support any platform operations"));
}

std::shared_ptr<mg::PlatformIPCPackage> mgx::IpcOperations::connection_ipc_package()
{
    CALLED
    auto package = std::make_shared<mg::PlatformIPCPackage>();
    package->ipc_fds.push_back(dup(drm->fd));

    std::cerr << "*********************** SENDING fd: " << package->ipc_fds[0] << " *****************************" << std::endl;
    return package;
}
